#pragma once
#include <core/filesystem/IFileHandle.h>
#include <core/log/ILog.h>
#include <core/fwd/IStringFwd.h>
#include <core/util/IPasskey.h>

namespace mk
{
enum class Result;
}; // namespace mk

namespace mk::log
{
// fwd declaration
struct LogContext;

class ISink
{
public:
    virtual ~ISink() = default;

    virtual void write(LogContext& ctx, StringView msg) = 0;
};

class ConsoleSink final: public ISink
{
public:
    ConsoleSink() = default;
    ~ConsoleSink() override = default;

    void write(LogContext& ctx, StringView msg) override;
};

class DebugSink final: public ISink
{
public:
    ~DebugSink() override = default;
    void write(LogContext& ctx, StringView msg) override;
};

class FileSink final: public ISink
{
    using FileSinkPasskey = util::Passkey<FileSink>;

public:
    static Result makeFileSink(const fs::Path& path, ISinkPtr& outPtr);
    FileSink(UniquePtr<fs::IFileHandle>&& handle, FileSinkPasskey);
    ~FileSink() override = default;

    void write(LogContext& ctx, StringView msg) override;

private:
    UniquePtr<fs::IFileHandle> file_;
};

} // namespace mk::log
