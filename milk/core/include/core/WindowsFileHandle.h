#pragma once
#include <core/filesystem/IFile.h>

#include <Windows.h>
namespace mk::fs
{

class WindowsFileHandle: public IFileHandle
{
public:
    WindowsFileHandle(HANDLE handle, usize size, FileAccessMode mode);
    ~WindowsFileHandle() override;

    // TODO(Cheese_S): rethink how these apis should work
    // Maybe we should allow the user to pass in a size
    // even for Vector<std::byte> or String?
    Result read(Vector<byte>& dst) override;
    Result read(String& dst) override;
    Result read(void* dst, usize dstSize) override;

    Result write(VectorView<u8> data) override;
    Result write(StringView str) override;
    Result write(const void* buf, usize size) override;

    inline usize size() override;

private:
    HANDLE         handle_;
    usize          size_;
    FileAccessMode mode_;
};

inline usize WindowsFileHandle::size()
{
    return size_;
}

}; // namespace mk::fs
