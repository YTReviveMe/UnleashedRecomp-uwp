#pragma once

#include <Windows.h>

typedef HANDLE(WINAPI* OpenFileForAppFn)(
    LPCWSTR, DWORD, DWORD, LPSECURITY_ATTRIBUTES, DWORD, DWORD, HANDLE);
typedef HANDLE(WINAPI* CreateFileMappingForAppFn)(
    HANDLE, LPSECURITY_ATTRIBUTES, ULONG, ULONG64, LPCWSTR);
typedef PVOID(WINAPI* MapViewOfFileForAppFn)(
    HANDLE, ULONG, ULONG64, SIZE_T);

static inline HANDLE OpenFileForApp(
    LPCWSTR fileName,
    DWORD desiredAccess,
    DWORD shareMode,
    LPSECURITY_ATTRIBUTES securityAttributes,
    DWORD creationDisposition,
    DWORD flagsAndAttributes,
    HANDLE templateFile)
{
    OpenFileForAppFn openFile = (OpenFileForAppFn)GetProcAddress(
        GetModuleHandleW(L"kernelbase.dll"), "CreateFileFromAppW");

    return openFile
        ? openFile(fileName, desiredAccess, shareMode, securityAttributes, creationDisposition, flagsAndAttributes, templateFile)
        : CreateFileW(fileName, desiredAccess, shareMode, securityAttributes, creationDisposition, flagsAndAttributes, templateFile);
}

static inline HANDLE CreateFileMappingForApp(
    HANDLE file,
    LPSECURITY_ATTRIBUTES securityAttributes,
    DWORD pageProtection,
    DWORD maximumSizeHigh,
    DWORD maximumSizeLow,
    LPCWSTR name)
{
    CreateFileMappingForAppFn createMapping = (CreateFileMappingForAppFn)GetProcAddress(
        GetModuleHandleW(L"kernelbase.dll"), "CreateFileMappingFromApp");

    return createMapping
        ? createMapping(file, securityAttributes, pageProtection,
            ((ULONG64)maximumSizeHigh << 32) | maximumSizeLow, name)
        : CreateFileMappingW(file, securityAttributes, pageProtection,
            maximumSizeHigh, maximumSizeLow, name);
}

static inline PVOID MapViewOfFileForApp(
    HANDLE fileMapping,
    DWORD desiredAccess,
    DWORD fileOffsetHigh,
    DWORD fileOffsetLow,
    SIZE_T numberOfBytesToMap)
{
    MapViewOfFileForAppFn mapView = (MapViewOfFileForAppFn)GetProcAddress(
        GetModuleHandleW(L"kernelbase.dll"), "MapViewOfFileFromApp");

    return mapView
        ? mapView(fileMapping, desiredAccess,
            ((ULONG64)fileOffsetHigh << 32) | fileOffsetLow, numberOfBytesToMap)
        : MapViewOfFile(fileMapping, desiredAccess,
            fileOffsetHigh, fileOffsetLow, numberOfBytesToMap);
}

#define CreateFileW OpenFileForApp
#define CreateFileMappingW CreateFileMappingForApp
#define MapViewOfFile MapViewOfFileForApp
