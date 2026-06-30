#pragma once
#include <core/IUniquePtr.h>
#include <core/log/ILog.h>
#include <core/concurrency/jobsystem/IJobSystem.h>
#include <swiss/render/Camera.h>
#include <core/util/IPasskey.h>

namespace mk::swiss
{
class PathTracer
{
    using PathTracerPasskey = util::Passkey<PathTracer>;

public:
    static Result makePathTracer(UniquePtr<PathTracer>& outPathTracer);
    PathTracer(PathTracerPasskey,
               UniquePtr<log::LogSystem>&&            logSystem,
               UniquePtr<cc::IJobSystem>&&            jobSystem,
               UniquePtr<render::PerspectiveCamera>&& camera);

    Result run();

    ~PathTracer();

private:
    UniquePtr<log::LogSystem>            logSystem_;
    UniquePtr<cc::IJobSystem>            jobSystem_;
    UniquePtr<render::PerspectiveCamera> camera_;
};
} // namespace mk::swiss
