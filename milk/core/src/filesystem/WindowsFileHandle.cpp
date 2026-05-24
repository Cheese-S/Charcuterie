#include <core/filesystem/IFile.h>
#include <core/container/IVector.h>
#include <core/container/IString.h>

#include <core/WindowsResult.h>
#include <core/WindowsFileHandle.h>
#include <core/IEnum.h>

namespace mk::fs
{
WindowsFileHandle::WindowsFileHandle(HANDLE handle, usize size, FileAccessMode mode):
    handle_(handle), size_(size), mode_(mode)
{
}

WindowsFileHandle::~WindowsFileHandle()
{
    CloseHandle(handle_);
}

Result WindowsFileHandle::read(Vector<std::byte>& dst)
{
    dst.resize(size_);
    return read(dst.data(), dst.size());
}

Result WindowsFileHandle::read(String& dst)
{
    dst.resize(size_);
    return read(dst.begin(), dst.size());
}

Result WindowsFileHandle::read(void* dst, usize dstSize)
{
    if (mode_ == FileAccessMode::eWrite || mode_ == FileAccessMode::eAppend)
    {
        return Result::eInvalidPermission;
    }

    if (dstSize < size_)
    {
        return Result::eOutOfCapactiy;
    }

    DWORD bytesRead;
    if (!ReadFile(handle_, dst, size_, &bytesRead, nullptr) || bytesRead != size_)
    {
        return winErrorToResult(GetLastError());
    }

    return Result::eOk;
}

Result WindowsFileHandle::write(StringView str)
{
    return write(str.data(), str.size());
}

Result WindowsFileHandle::write(VectorView<u8> data)
{
    return write(data.cbegin(), data.size());
}

Result WindowsFileHandle::write(const void* buf, usize size)
{
    if (mode_ == FileAccessMode::eRead)
    {
        return Result::eInvalidPermission;
    }

    DWORD bytesWritten = 0;
    if (!WriteFile(handle_, buf, size, &bytesWritten, nullptr) || bytesWritten != size)
    {
        return winErrorToResult(GetLastError());
    }

    return Result::eOk;
}

} // namespace mk::fs
