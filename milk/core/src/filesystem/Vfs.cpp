#include <core/filesystem/IVfs.h>
#include <core/filesystem/Vfs.h>
#include <core/filesystem/IPath.h>
#include <core/os/IWindows.h>
#include <core/container/IString.h>
#include <core/filesystem/WindowsFileHandle.h>
#include <core/filesystem/IAccessMode.h>
#include <core/WindowsResult.h>
#include <core/log/IRawLog.h>

namespace mk::fs
{

namespace
{
DWORD toAccessFlag(AccessMode mode)
{
    switch (mode)
    {
    case AccessMode::eRead:
        return GENERIC_READ;
    case AccessMode::eWrite:
        return GENERIC_WRITE;
    case AccessMode::eAppend:
        return FILE_APPEND_DATA;
    case AccessMode::eReadAndWrite:
    case AccessMode::eReadAndOverwrite:
        return GENERIC_READ | GENERIC_WRITE;
    case AccessMode::eReadAndAppend:
        return GENERIC_READ | FILE_APPEND_DATA;
    default:
        MK_ASSERT_UNREACHABLE();
        return GENERIC_READ;
    }
};

DWORD toCreationDeposition(AccessMode mode)
{
    switch (mode)
    {
    case AccessMode::eRead:
        return OPEN_EXISTING;
    case AccessMode::eWrite:
        return CREATE_ALWAYS;
    case AccessMode::eAppend:
        return OPEN_ALWAYS;
    case AccessMode::eReadAndWrite:
        return OPEN_EXISTING;
    case AccessMode::eReadAndOverwrite:
        return CREATE_ALWAYS;
    case AccessMode::eReadAndAppend:
        return OPEN_ALWAYS;
    default:
        MK_ASSERT_UNREACHABLE();
        return OPEN_EXISTING;
    }
};

} // namespace

Result getExePath(Path& outPath)
{
    static constexpr u32 kMaxPathLen = 2048;

    char buffer[kMaxPathLen];
    GetModuleFileName(nullptr, buffer, kMaxPathLen);
    Result res = winErrorToResult(GetLastError());
    if (isNotOk(res))
    {
        MK_RAW_LOG_ERROR("[vfs]: Exe path exceeds max supported length {}", kMaxPathLen);
        return res;
    }

    outPath = Path(buffer);
    return res;
}

Result Vfs::makeVfs(StringView projectName, UniquePtr<IVfs>& outVfs)
{
    Path root;
    if (isNotOk(getExePath(root)))
    {
        MK_RAW_LOG_ERROR("[vfs]: Failed to get exe path");
        return Result::eUnexpected;
    }

    // TODO(Cheese_S): This is arbitriary.
    root = root.parent().parent().parent().parent() / projectName;

    if (!CreateDirectory(root.cstr(), nullptr))
    {
        Result res = winErrorToResult(GetLastError());
        if (res != Result::eAlreadyExist)
        {
            MK_RAW_LOG_ERROR("[vfs]: Failed to create root directory {}, result: {}",
                             root.cstr(),
                             res);
            return Result::eUnexpected;
        }
        MK_RAW_LOG_INFO("[vfs]: project root initialized at: {}", root.cstr());
    }

    outVfs = makeUnique<Vfs>(std::move(root), VfsPasskey());
    return Result::eOk;
}

Vfs::Vfs(Path&& root, VfsPasskey): root_(root) {}

bool Vfs::fileExist(const Path& path) const
{
    return GetFileAttributes(toAbsolute(path).cstr()) != INVALID_FILE_ATTRIBUTES;
}

bool Vfs::dirExist(const Path& path) const
{
    DWORD attr = GetFileAttributes(toAbsolute(path).cstr());
    return attr != INVALID_FILE_ATTRIBUTES && (attr & FILE_ATTRIBUTE_ARCHIVE);
}

Result Vfs::openFile(const Path& path, AccessMode mode, UniquePtr<IFileHandle>& outHandle) const
{
    class RawHandleDeleter
    {
    public:
        void operator()(HANDLE* ptr)
        {
            if (ptr)
            {
                CloseHandle(*ptr);
            }
        }
    };

    HANDLE rawHandle = CreateFile(toAbsolute(path).cstr(),
                                  toAccessFlag(mode),
                                  FILE_SHARE_READ,
                                  nullptr,
                                  toCreationDeposition(mode),
                                  FILE_ATTRIBUTE_NORMAL,
                                  nullptr);

    if (rawHandle == INVALID_HANDLE_VALUE)
    {
        return winErrorToResult(GetLastError());
    }

    UniquePtr<HANDLE, RawHandleDeleter> osHandle(&rawHandle);
    LARGE_INTEGER                       size;
    if (!GetFileSizeEx(*osHandle, &size))
    {
        return winErrorToResult(GetLastError());
    }

    outHandle = makeUnique<WindowsFileHandle>(*osHandle.release(), size.QuadPart, mode);
    return Result::eOk;
}

Result Vfs::deleteFile(const Path& path) const
{
    if (DeleteFile(toAbsolute(path).cstr()))
    {
        return Result::eOk;
    }

    return winErrorToResult(GetLastError());
}

Result Vfs::createDir(const Path& path) const
{
    if (CreateDirectory(toAbsolute(path).cstr(), nullptr))
    {
        return Result::eOk;
    }

    return winErrorToResult(GetLastError());
}

Path Vfs::toAbsolute(const Path& vfsPath) const
{
    MK_ASSERTF(!vfsPath.isAbsolute(), "Incurring extra copy.");
    Path root(root_);
    return root / vfsPath;
}

} // namespace mk::fs
