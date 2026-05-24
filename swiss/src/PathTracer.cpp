#include <swiss/PathTracer.h>
#include <core/log/ISink.h>
#include <core/filesystem/IPath.h>

namespace mk::swiss
{

Result PathTracer::makePathTracer(PathTracerPasskey, UniquePtr<PathTracer>& outPathTracer)
{
    // init log system
    {
        FixedVector<log::ISinkPtr, 4> sinks;
        sinks.push(makeUnique<log::ConsoleSink>());
        sinks.push(makeUnique<log::DebugSink>());
        log::ISinkPtr fileSink;

        Result res = log::FileSink::makeFileSink(fs::Path("./log.txt"), fileSink);
        if (isNotOk(res))
        {
            MK_RAW_LOG_ERROR("Failed to")
            return res;
        };

        log::LogSystemConfig logSystemConfig = {
            .pattern = log::LogSystemConfig::kDefaultPattern,
            .
        };
    }

    return Result::eOk;
}

PathTracer::PathTracer() {}
} // namespace mk::swiss
