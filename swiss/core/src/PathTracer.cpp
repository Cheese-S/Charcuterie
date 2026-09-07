#include <core/filesystem/IPath.h>
#include <core/IAppContext.h>
#include <core/filesystem/IVfs.h>
#include <core/log/ISink.h>
#include <core/log/IFormatter.h>
#include <core/mlm/IRay.h>
#include <core/mlm/IFormatter.h>

#include <asset/import/builder/ITemplateAssetBuilder.h>
#include <asset/export/raw/IPpm.h>
#include <swiss/core/PathTracer.h>
#include <swiss/render/Bvh.h>

MK_ADD_AND_DEFINE_LOG_CATEGORY(PathTracer, "PathTracer");

namespace mk::swiss
{
// TODO(Cheese_S): probably should move this to the outer swiss app.
Result PathTracer::makePathTracer(UniquePtr<PathTracer>& outPathTracer)
{
    UniquePtr<fs::IVfs> vfs;
    {
        Result res = fs::Vfs::makeVfs("swiss_out", vfs);
        if (isNotOk(res))
        {
            MK_RAW_LOG_ERROR("Failed to initialize vfs");
            return res;
        }
    }
    AppContext<fs::IVfs>::registerIntsance(vfs.get());

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
        mlm::mat4    worldToCamera = mlm::mat4::translate(0, -1.0, 5.5);
        mlm::u16vec2 resolution(800, 600);
        camera = makeUnique<render::PerspectiveCamera>(
            worldToCamera,
            resolution,
            mlm::toFovY(mlm::kPiOver2, static_cast<f32>(resolution.x()) / resolution.y()),
            0.001,
            1000);
    }

    outPathTracer = makeUnique<PathTracer>(std::move(vfs),
                                           std::move(logSystem),
                                           std::move(jobSystem),
                                           std::move(camera),
                                           PathTracerPasskey());

    return Result::eOk;
}

PathTracer::PathTracer(UniquePtr<fs::IVfs>&&                  vfs,
                       UniquePtr<log::LogSystem>&&            logSystem,
                       UniquePtr<cc::IJobSystem>&&            jobSystem,
                       UniquePtr<render::PerspectiveCamera>&& camera,
                       PathTracerPasskey):
    vfs_(std::move(vfs)), logSystem_(std::move(logSystem)), jobSystem_(std::move(jobSystem)),
    camera_(std::move(camera))
{
}

PathTracer::~PathTracer()
{
    // TODO(Cheese_S): this is obviously wrong. singleton shouldn't be unregistered here
    AppContext<cc::IJobSystem>::unregisterInstance();
    AppContext<log::LogSystem>::unregisterInstance();
    AppContext<fs::IVfs>::unregisterInstance();
}

Result PathTracer::run()
{
    asset::ir::Ir ir;

    MK_LOG_ERROR_AND_RETURN_IF_NOT_OK(
        asset::TemplateAssetBuilder::build(fs::Path("../asset/bunny.glb"), ir),
        "Failed to build asset.");

    render::Bvh bvh(std::move(ir));

    // mlm::Sphere     sphere = { .center = mlm::PackedVec3(0, 0, 6), .r = 2.0f };
    mlm::PackedVec3 v2 = mlm::PackedVec3(-5, 0, 8);
    mlm::PackedVec3 v1 = mlm::PackedVec3(5, 0, 8);
    mlm::PackedVec3 v0 = mlm::PackedVec3(0, 5, 8);
    mlm::u16vec2    resolution = camera_->getResolution();

    Vector<f32> pixels;

    for (u16 y = 0; y < resolution.y(); y++)
    {
        MK_LOG_INFO("y: {}", y);
        for (u16 x = 0; x < resolution.x(); x++)
        {
            mlm::Ray ray = camera_->sampleRay(x, y);
            if (bvh.intersect(ray, kF32Infinity))
            {
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

    MK_LOG_ERROR_AND_RETURN_IF_NOT_OK(
        asset::exp::savePpm(fs::Path("result.ppm"), pixels, resolution.x(), resolution.y()),
        "Failed to write to ppm.");

    MK_LOG_INFO("Finished!");

    return Result::eOk;
}

} // namespace mk::swiss
