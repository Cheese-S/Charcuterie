#pragma once
#include <core/util/IPasskey.h>
#include <core/IUniquePtr.h>
#include <core/IResult.h>
#include <core/filesystem/IPath.h>

namespace mk::fs
{
class IFileHandle;
enum class AccessMode;

class IVfs
{
public:
    virtual ~IVfs() = default;

    virtual bool   fileExist(const Path& path) const = 0;
    virtual bool   dirExist(const Path& path) const = 0;
    virtual Result deleteFile(const Path& path) const = 0;
    virtual Result
    openFile(const Path& path, AccessMode mode, UniquePtr<IFileHandle>& outHandle) const = 0;

    // eAlreadyExist, if there is already a directory
    // eNotFound, if any intermediate directories dose not exist
    virtual Result createDir(const Path& path) const = 0;
    virtual Result deleteDir(const Path& path) const = 0;

    virtual Path resolve(const Path& vfsPath) const = 0;

    // virtual Result deleteDir(const Path& path) const = 0;
};

// TODO(Cheese_S): Maybe we can have a config that shows us how to map vfs paths to real fs apths
// TODO(Cheese_S): Api is not final yet.
// TODO(Cheese_S): Big problem: root = "C:/foo/bar", path = "/Games/aab", root / path = "Games/aab".
// TODO(Cheese_S): separate the interface the impl
// We need to fix this
class Vfs: public IVfs
{
    using VfsPasskey = ::mk::util::Passkey<Vfs>;

public:
    static Result makeVfs(StringView projectName, UniquePtr<IVfs>& outVfs);

    Vfs(Path&& root, VfsPasskey);
    ~Vfs() override = default;

    bool fileExist(const Path& path) const override;
    bool dirExist(const Path& path) const override;

    Result
    openFile(const Path& path, AccessMode mode, UniquePtr<IFileHandle>& outHandle) const override;

    Result deleteFile(const Path& path) const override;

    Result createDir(const Path& path) const override;
    Result deleteDir(const Path& path) const override;

    Path resolve(const Path& vfsPath) const override;

private:
    const Path root_;
};
}; // namespace mk::fs
