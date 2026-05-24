#include <core/log/Log.h>
#include <core/log/ISink.h>
#include <core/container/IString.h>
#include <core/IWindows.h>
#include <core/IEnum.h>
#include <core/log/IRawLog.h>
#include <core/log/AnsiColor.h>
#include <core/filesystem/IPath.h>

namespace mk::log
{

namespace
{

} // namespace

// ------------------------------ ConsoleSink ------------------------
void ConsoleSink::write(LogContext& ctx, StringView msg)
{
    AnsiColor color = toAnsiColor(ctx.level);
    FILE*     stream = stdout;

    std::fprintf(stream,
                 "%s%s%s",
                 toEscapeCode(color),
                 msg.data(),
                 toEscapeCode(AnsiColor::eReset));
    std::fflush(stream);
}

// ------------------------------ DebugSink ------------------------
void DebugSink::write(LogContext& ctx, StringView msg)
{
    MK_UNREF(ctx);
    OutputDebugStringA(msg.data());
}

// ------------------------------ FileSink ------------------------

Result FileSink::makeFileSink(const fs::Path& path, ISinkPtr& outPtr)
{
    UniquePtr<fs::IFileHandle> handle;
    Result res = fs::openUniqueFile(path, fs::FileAccessMode::eReadAndOverwrite, handle);
    if (isNotOk(res))
    {
        MK_RAW_LOG_ERROR("Failed to open file {}", path.cstr());
        return res;
    }
    outPtr = makeUnique<FileSink>(FileSinkPasskey{}, std::move(handle));
    return res;
}

FileSink::FileSink(FileSink::FileSinkPasskey, UniquePtr<fs::IFileHandle>&& handle):
    file_(std::move(handle))
{
}

void FileSink::write(LogContext&, StringView msg)
{
    if (isNotOk(file_->write(msg.begin(), msg.size())))
    {
        MK_RAW_LOG_ERROR("Failed to write to log file");
    }
}

} // namespace mk::log
