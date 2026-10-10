// Referenced from: https://github.com/xenia-canary/xenia-canary/blob/canary_experimental/src/xenia/vfs/devices/disc_image_device.cc

/**
 ******************************************************************************
 * Xenia : Xbox 360 Emulator Research Project                                 *
 ******************************************************************************
 * Copyright 2023 Ben Vanik. All rights reserved.                             *
 * Released under the BSD license - see LICENSE in the root for more details. *
 ******************************************************************************
 */

#pragma once

#include <filesystem>
#include <map>

#include "virtual_file_system.h"

#ifdef UNLEASHED_RECOMP_UWP
#include <Windows.h>
#else
#include <memory_mapped_file.h>
#endif

struct ISOFileSystem : VirtualFileSystem
{
#ifdef UNLEASHED_RECOMP_UWP
    HANDLE fileHandle = INVALID_HANDLE_VALUE;
    uint64_t fileSize = 0;
#else
    MemoryMappedFile mappedFile;
#endif
    std::map<std::string, std::tuple<size_t, size_t>> fileMap;
    std::string name;

    ISOFileSystem(const std::filesystem::path &isoPath);
    ~ISOFileSystem();
    bool load(const std::string &path, uint8_t *fileData, size_t fileDataMaxByteCount) const override;
    bool load(const std::string &path, size_t offset, uint8_t *fileData, size_t byteCount) const override;
    size_t getSize(const std::string &path) const override;
    bool exists(const std::string &path) const override;
    const std::string &getName() const override;
    bool empty() const;
    bool read(size_t offset, void *data, size_t size) const;

    static std::unique_ptr<ISOFileSystem> create(const std::filesystem::path &isoPath);
};
