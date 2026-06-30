#include <core/filesystem/IPath.h>
#include <core/log/ISink.h>
#include <core/log/IFormatter.h>
#include <asset/export/raw/IPpm.h>
#include <swiss/PathTracer.h>

MK_ADD_AND_DEFINE_LOG_CATEGORY(PathTracer, "PathTracer");

namespace mk::swiss
{
Result PathTracer::makePathTracer(UniquePtr<PathTracer>& outPathTracer)
{
    // init log system
    UniquePtr<log::LogSystem> logSystem;
    {
        FixedVector<log::ISinkPtr, 4> sinks;
        sinks.push(makeUnique<log::ConsoleSink>());
        sinks.push(makeUnique<log::DebugSink>());
        log::ISinkPtr fileSink;

        Result res = log::FileSink::makeFileSink(fs::Path("./log.txt"), fileSink);
        if (isNotOk(res))
        {
            MK_RAW_LOG_ERROR("Failed to create file sink");
            return res;
        };

        log::LogSystemConfig config = { .pattern = log::LogSystemConfig::kDefaultPattern,
                                        .sinks = sinks,
                                        .userFlagFormatters = {} };

        res = log::LogSystem::makeLogSystem(config, logSystem);
        if (isNotOk(res))
        {
            MK_RAW_LOG_ERROR("Failed to create the log system");
            return res;
        }
    }
    AppContext<log::LogSystem>::registerIntsance(logSystem.get());

    // init job system
    UniquePtr<cc::IJobSystem> jobSystem;
    {
        static constexpr cc::JobPoolSizeConfig kPoolSizeConfig = { .smallJobPoolSize = 4096,
                                                                   .mediumJobPoolSize = 1,
                                                                   .largeJobPoolSize = 1 };

        cc::JobSystemConfig systemConfig = {
            .numForegroundThreads = static_cast<u8>(std::thread::hardware_concurrency()),
            .numBackgorundThreads = 0,
        };

        jobSystem = makeUnique<cc::JobSystem<kPoolSizeConfig>>(systemConfig);
    }
    AppContext<cc::IJobSystem>::registerIntsance(jobSystem.get());

    UniquePtr<render::PerspectiveCamera> camera;
    {
        mlm::Transform worldToCamera;
        mlm::u16vec2   resolution(800, 600);
        camera = makeUnique<render::PerspectiveCamera>(
            worldToCamera,
            resolution,
            mlm::toFovY(mlm::kPiOver2, static_cast<f32>(resolution.x()) / resolution.y()),
            0.001,
            1000);
    }

    outPathTracer = makeUnique<PathTracer>(PathTracerPasskey{},
                                           std::move(logSystem),
                                           std::move(jobSystem),
                                           std::move(camera));

    return Result::eOk;
}

PathTracer::PathTracer(PathTracerPasskey,
                       UniquePtr<log::LogSystem>&&            logSystem,
                       UniquePtr<cc::IJobSystem>&&            jobSystem,
                       UniquePtr<render::PerspectiveCamera>&& camera):
    logSystem_(std::move(logSystem)), jobSystem_(std::move(jobSystem)), camera_(std::move(camera))
{
}

PathTracer::~PathTracer()
{
    AppContext<cc::IJobSystem>::unregisterInstance();
    AppContext<log::LogSystem>::unregisterInstance();
}

Result PathTracer::run()
{
    mlm::Sphere  sphere = { .center = mlm::Point(0, 0, 6), .r = 2.0f };
    mlm::u16vec2 resolution = camera_->getResolution();
    Vector<f32>  pixels;

    for (u16 y = 0; y < resolution.y(); y++)
    {
        for (u16 x = 0; x < resolution.x(); x++)
        {
            mlm::Ray ray = camera_->sampleRay(x, y);
            f32      t = 0;
            if (mlm::raySphereIntersection(ray, sphere, t))
            {
                MK_LOG_INFO("HIT: {}, {}", x, y);
                pixels.push(1.0f);
                pixels.push(1.0f);
                pixels.push(1.0f);
            }
            else
            {
                pixels.push(0.0f);
                pixels.push(0.0f);
                pixels.push(0.0f);
            }
        }
    }

    // mlm::Ray a = camera_->sampleRay(399, 0);
    // mlm::Ray b = camera_->sampleRay(400, 0);
    //
    // MK_LOG_DEBUG("a: {}, {}, {}, b: {}, {}, {}",
    //              a.d.x(),
    //              a.d.y(),
    //              a.d.z(),
    //              b.d.x(),
    //              b.d.y(),
    //              b.d.z());
    //
    fs::Path path("./result.ppm");
    MK_LOG_ERROR_AND_RETURN_IF_NOT_OK(
        asset::exp::savePpm(path, pixels, resolution.x(), resolution.y()),
        "Failed to write to ppm.");
    return Result::eOk;
}

} // namespace mk::swiss
