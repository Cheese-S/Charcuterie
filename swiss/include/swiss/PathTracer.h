#pragma once
#include <core/IUniquePtr.h>
#include <core/log/ILog.h>
#include <core/filesystem/IVfs.h>
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
    PathTracer(UniquePtr<fs::IVfs>&&                  vfs,
               UniquePtr<log::LogSystem>&&            logSystem,
               UniquePtr<cc::IJobSystem>&&            jobSystem,
               UniquePtr<render::PerspectiveCamera>&& camera,
               PathTracerPasskey);

    Result run();

    ~PathTracer();

private:
    UniquePtr<fs::IVfs>                  vfs_;
    UniquePtr<log::LogSystem>            logSystem_;
    UniquePtr<cc::IJobSystem>            jobSystem_;
    UniquePtr<render::PerspectiveCamera> camera_;
};
} // namespace mk::swiss
