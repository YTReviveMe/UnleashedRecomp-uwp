#pragma once

#include <mod/mod_loader.h>

#define USER_DIRECTORY "UnleashedRecomp"

#ifndef GAME_INSTALL_DIRECTORY
#define GAME_INSTALL_DIRECTORY "E:\\Unleashed"
#endif

inline constexpr wchar_t UWP_DEVELOPMENT_STORAGE_ROOT[] = L"D:\\DevelopmentFiles";
inline constexpr wchar_t UWP_GAME_STORAGE_DIRECTORY[] = L"Unleashed";

extern std::filesystem::path g_executableRoot;
extern std::filesystem::path g_gamePath;

bool CheckPortable();
std::filesystem::path BuildUserPath();
const std::filesystem::path& GetUserPath();
const std::filesystem::path& GetGamePath();
bool IsUserPathReady();
void SetGamePath(const std::filesystem::path& path, bool remember = true);
void InitializeStoragePaths();

inline std::filesystem::path GetSavePath(bool checkForMods)
{
    if (checkForMods && !ModLoader::s_saveFilePath.empty())
        return ModLoader::s_saveFilePath.parent_path();
    else
        return GetUserPath() / "save";
}

inline std::filesystem::path GetLogPath()
{
    return GetUserPath() / "logs";
}

inline std::filesystem::path GetModsPath()
{
    return GetUserPath() / "mods";
}

// Returned file name may not necessarily be
// equal to SYS-DATA as mods can assign anything.
inline std::filesystem::path GetSaveFilePath(bool checkForMods)
{
    if (checkForMods && !ModLoader::s_saveFilePath.empty())
        return ModLoader::s_saveFilePath;
    else
        return GetSavePath(false) / "SYS-DATA";
}
