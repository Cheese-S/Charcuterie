#include <core/IAppContext.h>
#include <core/filesystem/IVfs.h>
#include <core/filesystem/IFileHandle.h>
#include <core/filesystem/IAccessMode.h>
#include <core/filesystem/IFileUtil.h>

#include <core/log/ILog.h>
#include <core/log/ILogCategory.h>

MK_DEFINE_DEFAULT_LOG_CATEGORY(FileSystem);

namespace mk::fs::util
{

Result readBinaryFile(const fs::Path& path, Vector<byte>& outBytes)
{
    MK_ASSERTF(outBytes.size(), "Passing non-empty out bytes to readBinaryFile");

    fs::IVfs& vfs = AppContext<fs::IVfs>::get();

    UniquePtr<fs::IFileHandle> file;
    MK_LOG_ERROR_AND_RETURN_IF_NOT_OK(vfs.openFile(path, fs::AccessMode::eRead, file),
                                      "Failed to open file: {}",
                                      path.cstr());

    MK_LOG_ERROR_AND_RETURN_IF_NOT_OK(file->read(outBytes),
                                      "Failed to read bytes from file: {}",
                                      path.cstr());

    return Result::eOk;
}

Result writeBinaryFile(const fs::Path& path, const Vector<byte>& bytes)
{
    MK_ASSERTF(bytes.size(), "Passing empty bytes to writeBinaryFile");

    fs::IVfs& vfs = AppContext<fs::IVfs>::get();

    UniquePtr<fs::IFileHandle> file;
    MK_LOG_ERROR_AND_RETURN_IF_NOT_OK(vfs.openFile(path, fs::AccessMode::eWrite, file),
                                      "Failed to open file: {}",
                                      path.cstr());

    MK_LOG_ERROR_AND_RETURN_IF_NOT_OK(file->write(bytes),
                                      "Failed to write bytes to file: {}",
                                      path.cstr());

    return Result::eOk;
}

} // namespace mk::fs::util
