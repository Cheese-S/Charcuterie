#include <core/IAppContext.h>
#include <core/filesystem/IVfs.h>
#include <core/log/ILog.h>

#include <asset/import/builder/ITemplateAssetBuilder.h>
#include <asset/import/builder/GltfIrBuilder.h>
#include <asset/LogCategory.h>

MK_DEFINE_DEFAULT_LOG_CATEGORY(Asset);

namespace mk::asset
{

Result TemplateAssetBuilder::build(const fs::Path& path, ir::Ir& outIr)
{
    StringView suffix = path.suffix();

    MK_LOG_ERROR_AND_RETURN_RET_IF_TRUE((suffix != "gltf" && suffix != "glb"),
                                        Result::eInvalidParam,
                                        "Only gltf files are supported.");

    fs::Path      owned = path;
    GltfIrBuilder gltfBuilder(std::move(owned));
    MK_LOG_ERROR_AND_RETURN_IF_NOT_OK(gltfBuilder.build(outIr), "Failed to build ir from gltf.");

    return Result::eOk;
}
} // namespace mk::asset
