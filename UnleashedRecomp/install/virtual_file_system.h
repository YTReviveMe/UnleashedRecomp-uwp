#pragma once

#include <algorithm>
#include <memory>

struct VirtualFileSystem {
    virtual ~VirtualFileSystem() { };
    virtual bool load(const std::string &path, uint8_t *fileData, size_t fileDataMaxByteCount) const = 0;
    virtual bool load(const std::string &path, size_t offset, uint8_t *fileData, size_t byteCount) const
    {
        size_t fileSize = getSize(path);
        if (offset > fileSize || byteCount > fileSize - offset)
            return false;

        std::vector<uint8_t> contents(fileSize);
        if (!load(path, contents.data(), contents.size()))
            return false;

        std::memcpy(fileData, contents.data() + offset, byteCount);
        return true;
    }
    virtual size_t getSize(const std::string &path) const = 0;
    virtual bool exists(const std::string &path) const = 0;
    virtual const std::string &getName() const = 0;

    // Concrete implementation shortcut.
    bool load(const std::string &path, std::vector<uint8_t> &fileData)
    {
        size_t fileDataSize = getSize(path);
        if (fileDataSize == 0)
        {
            return false;
        }

        fileData.resize(fileDataSize);
        return load(path, fileData.data(), fileDataSize);
    }
};
