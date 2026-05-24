#pragma once
#include <core/IType.h>
#include <core/IResult.h>
#include <core/fwd/IStringFwd.h>
#include <core/fwd/IVectorFwd.h>
#include <core/IUniquePtr.h>
#include <core/ISharedPtr.h>

namespace mk::fs
{
class Path;

// TODO(Cheese_S): convert this to bit flags
enum class FileAccessMode : int
{
    eRead = 0, // "r", file must exist
    eWrite,    // "w", create a new file if does not exist
    eAppend,   // "a", create a new file if does not exist

    eReadAndWrite,     // "r+", file must exist
    eReadAndOverwrite, // "w+", create a new file if does not exist
    eReadAndAppend,    // "a+", create a new file if does not exist
};

enum class FileType
{
    eFile,
    eDirectory,
};

struct FileStats
{
    FileType type;
};

class IFileHandle
{
public:
    virtual ~IFileHandle() = default;

    [[nodiscard]] virtual Result read(Vector<std::byte>& dst) = 0;
    [[nodiscard]] virtual Result read(String& dst) = 0;
    [[nodiscard]] virtual Result read(void* dst, usize dstSize) = 0;

    [[nodiscard]] virtual Result write(VectorView<u8> data) = 0;
    [[nodiscard]] virtual Result write(const void* buf, usize size) = 0;
    [[nodiscard]] virtual Result write(StringView str) = 0;
    virtual usize                size() = 0;
};

using IFileHandlePtr = UniquePtr<IFileHandle>;

Result
openUniqueFile(const Path& path, FileAccessMode mode, UniquePtr<IFileHandle>& outHandle);

Result
openSharedFile(const Path& path, FileAccessMode mode, SharedPtr<IFileHandle>& outHandle);

Result removeFile(const Path& path);

bool fileExist(const Path& path);

} // namespace mk::fs
