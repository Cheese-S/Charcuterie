
#include <core/IAssert.h>
#include <core/filesystem/IFile.h>
#include <core/container/IString.h>

#include <core/WindowsResult.h>
#include <core/WindowsFileHandle.h>

#include <core/filesystem/IPath.h>

namespace
{
DWORD fileAccessModeToAccessFlag(mk::fs::FileAccessMode mode)
{
    switch (mode)
    {
    case mk::fs::FileAccessMode::eRead:
        return GENERIC_READ;
    case mk::fs::FileAccessMode::eWrite:
        return GENERIC_WRITE;
    case mk::fs::FileAccessMode::eAppend:
        return FILE_APPEND_DATA;
    case mk::fs::FileAccessMode::eReadAndWrite:
    case mk::fs::FileAccessMode::eReadAndOverwrite:
        return GENERIC_READ | GENERIC_WRITE;
    case mk::fs::FileAccessMode::eReadAndAppend:
        return GENERIC_READ | FILE_APPEND_DATA;
    default:
        MK_ASSERT(false);
        return GENERIC_READ;
    }
};

DWORD fileAccessModeToCreationDeposition(mk::fs::FileAccessMode mode)
{
    switch (mode)
    {
    case mk::fs::FileAccessMode::eRead:
        return OPEN_EXISTING;
    case mk::fs::FileAccessMode::eWrite:
        return CREATE_ALWAYS;
    case mk::fs::FileAccessMode::eAppend:
        return OPEN_ALWAYS;
    case mk::fs::FileAccessMode::eReadAndWrite:
        return OPEN_EXISTING;
    case mk::fs::FileAccessMode::eReadAndOverwrite:
        return CREATE_ALWAYS;
    case mk::fs::FileAccessMode::eReadAndAppend:
        return OPEN_ALWAYS;
    default:
        MK_ASSERT(false);
        return OPEN_EXISTING;
    }
};

} // namespace

namespace mk::fs
{

namespace
{

Result openFile(const Path& path, FileAccessMode mode, HANDLE& outHandle, u64& outSize)
{
    HANDLE handle = CreateFile(path.cstr(),
                               fileAccessModeToAccessFlag(mode),
                               FILE_SHARE_READ,
                               nullptr,
                               fileAccessModeToCreationDeposition(mode),
                               FILE_ATTRIBUTE_NORMAL,
                               nullptr);

    if (handle == INVALID_HANDLE_VALUE)
    {
        goto failed_create_file;
    }

    LARGE_INTEGER size;
    if (!GetFileSizeEx(handle, &size))
    {
        goto failed_get_size;
    }

    outHandle = handle;
    outSize = size.QuadPart;

    return Result::eOk;

failed_get_size:
    CloseHandle(handle);
failed_create_file:
    return winErrorToResult(GetLastError());
}

} // namespace

Result
openUniqueFile(const Path& path, FileAccessMode mode, UniquePtr<IFileHandle>& outHandle)
{
    HANDLE handle;
    u64    size;
    MK_RETURN_IF_NOT_OK(openFile(path, mode, handle, size));

    outHandle = makeUnique<WindowsFileHandle>(handle, size, mode);
    return Result::eOk;
}

Result
openSharedFile(const Path& path, FileAccessMode mode, SharedPtr<IFileHandle>& outHandle)
{
    HANDLE handle;
    u64    size;
    MK_RETURN_IF_NOT_OK(openFile(path, mode, handle, size));

    outHandle = makeShared<WindowsFileHandle>(handle, size, mode);
    return Result::eOk;
}

bool fileExist(const Path& path)
{
    return GetFileAttributes(path.cstr()) != INVALID_FILE_ATTRIBUTES;
}

Result removeFile(const Path& path)
{
    if (DeleteFile(path.cstr()))
    {
        return Result::eOk;
    }

    return winErrorToResult(GetLastError());
}

} // namespace mk::fs
