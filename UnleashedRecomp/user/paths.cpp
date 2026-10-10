#include "paths.h"
#include <install/installer.h>
#include <os/process.h>

#ifdef UNLEASHED_RECOMP_UWP
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Foundation.Collections.h>
#include <winrt/Windows.Storage.h>
#endif

std::filesystem::path g_executableRoot = os::process::GetExecutablePath().remove_filename();
std::filesystem::path g_gamePath;
std::filesystem::path g_userPath;
static bool g_userPathReady = false;

#ifdef UNLEASHED_RECOMP_UWP
static constexpr wchar_t StoragePathSetting[] = L"GameStoragePath";

static std::filesystem::path GetLegacyLocalStatePath()
{
    using namespace winrt::Windows::Storage;
    return std::filesystem::path(ApplicationData::Current().LocalFolder().Path().c_str()) / USER_DIRECTORY;
}

static std::filesystem::path GetInternalGameStoragePath()
{
    return std::filesystem::path(UWP_DEVELOPMENT_STORAGE_ROOT) / UWP_GAME_STORAGE_DIRECTORY;
}

static bool IsValidGameStoragePath(const std::filesystem::path& path)
{
    if (path.empty())
        return false;

    try
    {
        std::filesystem::path modulePath;
        return Installer::checkGameInstall(path, modulePath);
    }
    catch (const std::filesystem::filesystem_error&)
    {
        return false;
    }
}

static std::filesystem::path FindPreparedGameStoragePath(
    const winrt::Windows::Storage::StorageFolder& root)
{
    using namespace winrt;
    using namespace winrt::Windows::Storage;

    try
    {
        auto item = root.TryGetItemAsync(UWP_GAME_STORAGE_DIRECTORY).get();
        if (!item || !item.IsOfType(StorageItemTypes::Folder))
            return {};

        const std::filesystem::path candidate(item.Path().c_str());
        return IsValidGameStoragePath(candidate) ? candidate : std::filesystem::path{};
    }
    catch (const hresult_error&)
    {
        return {};
    }
}

static std::filesystem::path FindExternalGameStoragePath()
{
    using namespace winrt;
    using namespace winrt::Windows::Storage;

    try
    {
        auto roots = KnownFolders::RemovableDevices().GetFoldersAsync().get();
        std::vector<StorageFolder> sortedRoots(roots.begin(), roots.end());
        std::sort(sortedRoots.begin(), sortedRoots.end(), [](const StorageFolder& left, const StorageFolder& right)
        {
            return _wcsicmp(left.Path().c_str(), right.Path().c_str()) < 0;
        });

        for (const auto& root : sortedRoots)
        {
            if (auto path = FindPreparedGameStoragePath(root); !path.empty())
                return path;
        }
    }
    catch (const hresult_error&)
    {
        return {};
    }

    return {};
}

static bool PathIsWithin(std::filesystem::path child, std::filesystem::path parent)
{
    std::wstring childText = child.lexically_normal().wstring();
    std::wstring parentText = parent.lexically_normal().wstring();
    std::transform(childText.begin(), childText.end(), childText.begin(), ::towlower);
    std::transform(parentText.begin(), parentText.end(), parentText.begin(), ::towlower);
    const std::wstring parentBase = parentText;
    if (!parentText.empty() && parentText.back() != L'\\')
        parentText.push_back(L'\\');

    return childText == parentBase ||
        (childText.size() >= parentText.size() && childText.compare(0, parentText.size(), parentText) == 0);
}

static bool HasLegacyGameStorageName(const std::filesystem::path& path)
{
    return _wcsicmp(path.filename().c_str(), L"UnleashedRecomp") == 0;
}
#endif

bool CheckPortable()
{
    return std::filesystem::exists(g_executableRoot / "portable.txt");
}

std::filesystem::path BuildUserPath()
{
#ifdef UNLEASHED_RECOMP_UWP
    return GetInternalGameStoragePath();
#else
    if (CheckPortable())
        return g_executableRoot;

    std::filesystem::path userPath;

#if defined(_WIN32)
    PWSTR knownPath = NULL;
    if (SHGetKnownFolderPath(FOLDERID_RoamingAppData, 0, NULL, &knownPath) == S_OK)
        userPath = std::filesystem::path{ knownPath } / USER_DIRECTORY;

    CoTaskMemFree(knownPath);
#elif defined(__linux__)
    const char* homeDir = getenv("HOME");
    if (homeDir == nullptr)
    {
        homeDir = getpwuid(getuid())->pw_dir;
    }

    if (homeDir != nullptr)
    {
        // Prefer to store in the .config directory if it exists. Use the home directory otherwise.
        std::filesystem::path homePath = homeDir;
        std::filesystem::path configPath = homePath / ".config";
        if (std::filesystem::exists(configPath))
            userPath = configPath / USER_DIRECTORY;
        else
            userPath = homePath / ("." USER_DIRECTORY);
    }
#else
    static_assert(false, "GetUserPath() not implemented for this platform.");
#endif

    return userPath;
#endif
}

const std::filesystem::path& GetUserPath()
{
    return g_userPath;
}

const std::filesystem::path& GetGamePath()
{
    return g_gamePath;
}

bool IsUserPathReady()
{
    return g_userPathReady;
}

void SetGamePath(const std::filesystem::path& path, bool remember)
{
    if (path.empty())
        return;

    g_gamePath = path.lexically_normal();
    g_userPathReady = true;

#ifdef UNLEASHED_RECOMP_UWP
    g_userPath = g_gamePath;

    std::error_code errorCode;
    std::filesystem::create_directories(GetSavePath(false), errorCode);
    errorCode.clear();
    std::filesystem::create_directories(GetLogPath(), errorCode);
    errorCode.clear();
    std::filesystem::create_directories(GetModsPath(), errorCode);

    if (remember)
    {
        auto values = winrt::Windows::Storage::ApplicationData::Current().LocalSettings().Values();
        values.Insert(StoragePathSetting, winrt::box_value(g_gamePath.wstring()));
    }
#else
    g_userPath = g_gamePath;
    (void)remember;
#endif
}

void InitializeStoragePaths()
{
#ifdef UNLEASHED_RECOMP_UWP
    std::filesystem::path selectedPath;
    auto values = winrt::Windows::Storage::ApplicationData::Current().LocalSettings().Values();
    if (auto value = values.TryLookup(StoragePathSetting))
    {
        try
        {
            selectedPath = std::filesystem::path(winrt::unbox_value<winrt::hstring>(value).c_str());
        }
        catch (const winrt::hresult_error&)
        {
            selectedPath.clear();
        }
    }

    const auto oldLocalStatePath = GetLegacyLocalStatePath();
    const bool wasOldInternalPath = PathIsWithin(selectedPath, oldLocalStatePath);
    const auto internalPath = GetInternalGameStoragePath();

    if (!wasOldInternalPath && !HasLegacyGameStorageName(selectedPath) &&
        IsValidGameStoragePath(selectedPath))
    {
        SetGamePath(selectedPath, false);
        return;
    }

    if (IsValidGameStoragePath(internalPath))
    {
        SetGamePath(internalPath, false);
        return;
    }

    if (const auto externalPath = FindExternalGameStoragePath(); !externalPath.empty())
    {
        SetGamePath(externalPath);
        return;
    }

    SetGamePath(internalPath, false);
    g_userPathReady = false;
#else
    g_gamePath = GAME_INSTALL_DIRECTORY;
    g_userPath = BuildUserPath();
    g_userPathReady = true;
#endif
}
