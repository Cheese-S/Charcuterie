#define CGLTF_IMPLEMENTATION
#include <cgltf.h>
#undef CGLTF_IMPLEMENTATION

#include <core/filesystem/IFile.h>
#include <core/container/IVector.h>
#include <core/IAppContext.h>
#include <core/log/ILog.h>
#include <core/log/ILogCategory.h>
#include <core/IEnum.h>

#include <asset/import/raw/Gltf.h>
#include <asset/definition/MeshDefinition.h>

MK_DEFINE_DEFAULT_LOG_CATEGORY(Asset);

namespace
{
void* cgltfMkAllocBridge(void*, cgltf_size size)
{
    return mk::mm::alloc(size, 16);
}

void cgltfMkFreeBridge(void*, void* ptr)
{
    return mk::mm::free(ptr);
}
} // namespace

namespace mk::asset
{

namespace
{
Result
parseGltf(const fs::Path& path, const cgltf_data& gltf, MeshDefinition& outDefinition)
{
    MK_UNREF(gltf);
    MK_UNREF(outDefinition);
    MK_UNREF(path);

    // // parse the default / [0] scene for now;
    cgltf_scene* scene;
    if (gltf.scene)
    {
        scene = gltf.scene;
    }
    else if (gltf.scenes_count > 0)
    {
        scene = &gltf.scenes[0];
    }
    else
    {
        MK_LOG_ERROR("Missing scene in {}", path.cstr());
        return Result::eInvalidParam;
    }

    if (!scene->nodes_count)
    {
        MK_LOG_ERROR("Missing nodes in scene {}, file {}", scene->name, path.cstr());
        return Result::eInvalidParam;
    }

    cgltf_node* node;
    for (cgltf_size i = 0; i < scene->nodes_count; i++)
    {
        node = scene->nodes[i];
        node->children_count
    }

    return Result::eOk;
}
} // namespace

Result importGltf(const fs::Path& path, MeshDefinition& outDefinition)
{
    UniquePtr<fs::IFileHandle> file;
    MK_LOG_ERROR_AND_RETURN_IF_NOT_OK(
        fs::openUniqueFile(path, fs::FileAccessMode::eRead, file),
        "Failed to open gltf file: {}.",
        path.cstr());

    Vector<std::byte> bytes;
    MK_LOG_ERROR_AND_RETURN_IF_NOT_OK(file->read(bytes),
                                      "Failed to read bytes from gltf file: {}",
                                      path.cstr());

    cgltf_options options = {};
    options.memory.alloc_func = cgltfMkAllocBridge;
    options.memory.free_func = cgltfMkFreeBridge;
    options.memory.user_data = nullptr;

    cgltf_data*  gltf;
    cgltf_result cgltfResult = cgltf_parse(&options, bytes.cdata(), file->size(), &gltf);
    MK_LOG_ERROR_AND_RETURN_RET_IF_FALSE(
        cgltfResult == cgltf_result_success,
        Result::eOk,
        "Failed to parse gltf file. cgltf error code: {}",
        enum_utils::toUnderlying(cgltfResult));

    Result result = parseGltf(path, *gltf, outDefinition);
    cgltf_free(gltf);
    return result;
}

} // namespace mk::asset
