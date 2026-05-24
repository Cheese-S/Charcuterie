#include <asset/import/builder/ITemplateAssetBuilder.h>
#include <asset/import/raw/Gltf.h>

namespace mk::asset
{
Result TemplateAssetBuilder::build(const fs::Path& path, MeshDefinition& outDefinition)
{
    StringView suffix = path.suffix();
    if (suffix == "gltf" || suffix == "glb")
    {
        return importGltf(path, outDefinition);
    }

    return Result::eOk;
}
} // namespace mk::asset
