// Referenced from: https://github.com/xenia-canary/xenia-canary/blob/canary_experimental/src/xenia/vfs/devices/disc_image_device.cc

/**
 ******************************************************************************
 * Xenia : Xbox 360 Emulator Research Project                                 *
 ******************************************************************************
 * Copyright 2023 Ben Vanik. All rights reserved.                             *
 * Released under the BSD license - see LICENSE in the root for more details. *
 ******************************************************************************
 */

#include "iso_file_system.h"

#include <stack>

#ifdef UNLEASHED_RECOMP_UWP
#include "uwp_file_api.h"
#endif

ISOFileSystem::ISOFileSystem(const std::filesystem::path &isoPath)
{
#ifdef UNLEASHED_RECOMP_UWP
    fileHandle = OpenFileForApp(isoPath.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr,
        OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    LARGE_INTEGER size{};
    if (fileHandle == INVALID_HANDLE_VALUE || !GetFileSizeEx(fileHandle, &size))
    {
        if (fileHandle != INVALID_HANDLE_VALUE)
            CloseHandle(fileHandle);
        fileHandle = INVALID_HANDLE_VALUE;
        return;
    }
    fileSize = size.QuadPart;
#else
    mappedFile.open(isoPath);
    if (!mappedFile.isOpen())
    {
        return;
    }
#endif

    name = (const char *)(isoPath.filename().u8string().data());

    // Find root sector.
    uint32_t gameOffset = 0;
    const size_t XeSectorSize = 2048;
    static const size_t PossibleOffsets[] = { 0x00000000, 0x0000FB20, 0x00020600, 0x02080000, 0x0FD90000, };
    bool magicFound = false;
    const char RefMagic[] = "MICROSOFT*XBOX*MEDIA";
    for (size_t i = 0; i < std::size(PossibleOffsets); i++)
    {
        size_t fileOffset = PossibleOffsets[i] + (32 * XeSectorSize);
        char magic[sizeof(RefMagic) - 1];
        if (read(fileOffset, magic, sizeof(magic)) &&
            std::memcmp(magic, RefMagic, sizeof(magic)) == 0)
        {
            gameOffset = PossibleOffsets[i];
            magicFound = true;
        }
    }

    size_t rootInfoOffset = gameOffset + (32 * XeSectorSize) + 20;
    uint32_t rootInfo[2];
    if (!magicFound || !read(rootInfoOffset, rootInfo, sizeof(rootInfo)))
    {
#ifdef UNLEASHED_RECOMP_UWP
        CloseHandle(fileHandle);
        fileHandle = INVALID_HANDLE_VALUE;
#else
        mappedFile.close();
#endif
        return;
    }

    // Parse root information.
    uint32_t rootSector = rootInfo[0];
    uint32_t rootSize = rootInfo[1];
    size_t rootOffset = gameOffset + (rootSector * XeSectorSize);
    const uint32_t MinRootSize = 13;
    const uint32_t MaxRootSize = 32 * 1024 * 1024;
    if ((rootSize < MinRootSize) || (rootSize > MaxRootSize))
    {
#ifdef UNLEASHED_RECOMP_UWP
        CloseHandle(fileHandle);
        fileHandle = INVALID_HANDLE_VALUE;
#else
        mappedFile.close();
#endif
        return;
    }

    struct IterationStep
    {
        std::string fileNameBase;
        size_t nodeOffset = 0;
        size_t entryOffset = 0;

        IterationStep() = default;
        IterationStep(std::string fileNameBase, size_t nodeOffset, size_t entryOffset) : fileNameBase(fileNameBase), nodeOffset(nodeOffset), entryOffset(entryOffset) { }
    };

    std::stack<IterationStep> iterationStack;
    iterationStack.emplace("", rootOffset, 0);

    IterationStep step;
    uint16_t nodeL, nodeR;
    uint32_t sector, length;
    uint8_t attributes, nameLength;
    char fileName[256];
    const uint8_t FileAttributeDirectory = 0x10;
    while (!iterationStack.empty())
    {
        step = iterationStack.top();
        iterationStack.pop();

        size_t infoOffset = step.nodeOffset + step.entryOffset;
        uint8_t info[14];
        if (!read(infoOffset, info, sizeof(info)))
        {
#ifdef UNLEASHED_RECOMP_UWP
            CloseHandle(fileHandle);
            fileHandle = INVALID_HANDLE_VALUE;
#else
            mappedFile.close();
#endif
            return;
        }

        std::memcpy(&nodeL, info + 0, sizeof(nodeL));
        std::memcpy(&nodeR, info + 2, sizeof(nodeR));
        std::memcpy(&sector, info + 4, sizeof(sector));
        std::memcpy(&length, info + 8, sizeof(length));
        attributes = info[12];
        nameLength = info[13];

        size_t nameOffset = infoOffset + 14;
        if (!read(nameOffset, fileName, nameLength))
        {
#ifdef UNLEASHED_RECOMP_UWP
            CloseHandle(fileHandle);
            fileHandle = INVALID_HANDLE_VALUE;
#else
            mappedFile.close();
#endif
            return;
        }

        fileName[nameLength] = '\0';

        if (nodeL)
        {
            iterationStack.emplace(step.fileNameBase, step.nodeOffset, nodeL * 4);
        }

        if (nodeR)
        {
            iterationStack.emplace(step.fileNameBase, step.nodeOffset, nodeR * 4);
        }

        std::string fileNameUTF8 = step.fileNameBase + fileName;
        if (attributes & FileAttributeDirectory)
        {
            if (length > 0)
            {
                iterationStack.emplace(fileNameUTF8 + "/", gameOffset + sector * XeSectorSize, 0);
            }
        }
        else
        {
            fileMap[fileNameUTF8] = { gameOffset + sector * XeSectorSize, length};
        }
    }
}

ISOFileSystem::~ISOFileSystem()
{
#ifdef UNLEASHED_RECOMP_UWP
    if (fileHandle != INVALID_HANDLE_VALUE)
        CloseHandle(fileHandle);
#endif
}

bool ISOFileSystem::read(size_t offset, void *data, size_t size) const
{
#ifdef UNLEASHED_RECOMP_UWP
    if (fileHandle == INVALID_HANDLE_VALUE || offset > fileSize || size > fileSize - offset)
        return false;

    LARGE_INTEGER position{};
    position.QuadPart = offset;
    if (!SetFilePointerEx(fileHandle, position, nullptr, FILE_BEGIN))
        return false;

    auto destination = static_cast<uint8_t *>(data);
    while (size != 0)
    {
        DWORD chunkSize = static_cast<DWORD>(std::min<size_t>(size, 16 * 1024 * 1024));
        DWORD bytesRead = 0;
        if (!ReadFile(fileHandle, destination, chunkSize, &bytesRead, nullptr) || bytesRead != chunkSize)
            return false;
        destination += bytesRead;
        size -= bytesRead;
    }
    return true;
#else
    if (offset > mappedFile.size() || size > mappedFile.size() - offset)
        return false;
    std::memcpy(data, mappedFile.data() + offset, size);
    return true;
#endif
}

bool ISOFileSystem::load(const std::string &path, uint8_t *fileData, size_t fileDataMaxByteCount) const
{
    auto it = fileMap.find(path);
    if (it != fileMap.end())
    {
        if (fileDataMaxByteCount < std::get<1>(it->second))
        {
            return false;
        }

        return read(std::get<0>(it->second), fileData, std::get<1>(it->second));
    }
    else
    {
        return false;
    }
}

bool ISOFileSystem::load(const std::string &path, size_t offset, uint8_t *fileData, size_t byteCount) const
{
    auto it = fileMap.find(path);
    if (it == fileMap.end())
        return false;

    const size_t fileOffset = std::get<0>(it->second);
    const size_t fileSize = std::get<1>(it->second);
    if (offset > fileSize || byteCount > fileSize - offset)
        return false;

    return read(fileOffset + offset, fileData, byteCount);
}

size_t ISOFileSystem::getSize(const std::string &path) const
{
    auto it = fileMap.find(path);
    if (it != fileMap.end())
    {
        return std::get<1>(it->second);
    }
    else
    {
        return 0;
    }
}

bool ISOFileSystem::exists(const std::string &path) const
{
    return fileMap.find(path) != fileMap.end();
}

const std::string &ISOFileSystem::getName() const
{
    return name;
}

bool ISOFileSystem::empty() const 
{
#ifdef UNLEASHED_RECOMP_UWP
    return fileHandle == INVALID_HANDLE_VALUE;
#else
    return !mappedFile.isOpen();
#endif
}

std::unique_ptr<ISOFileSystem> ISOFileSystem::create(const std::filesystem::path &isoPath) {
    std::unique_ptr<ISOFileSystem> isoFs = std::make_unique<ISOFileSystem>(isoPath);
    if (!isoFs->empty())
    {
        return isoFs;
    }
    else
    {
        return nullptr;
    }
}
