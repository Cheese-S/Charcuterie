#pragma once
#include <core/IUniquePtr.h>
#include <core/log/ILog.h>
#include <core/concurrency/jobsystem/IJobSystem.h>
#include <core/util/IPasskey.h>

namespace mk::swiss
{
class PathTracer
{
    using PathTracerPasskey = util::Passkey<PathTracer>;

public:
    static Result makePathTracer(PathTracerPasskey, UniquePtr<PathTracer>& outPathTracer);

private:
    PathTracer();

    UniquePtr<log::LogSystem> logSystem_;
    UniquePtr<cc::IJobSystem> jobSystem_;
};
} // namespace mk::swiss
