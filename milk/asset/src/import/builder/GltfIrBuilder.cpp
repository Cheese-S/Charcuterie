#define CGLTF_IMPLEMENTATION
#include <cgltf.h>
#undef CGLTF_IMPLEMENTATION

#include <core/filesystem/IFileUtil.h>

#include <core/container/IVector.h>
#include <core/IAppContext.h>
#include <core/filesystem/IVfs.h>
#include <core/log/ILog.h>
#include <core/log/ILogCategory.h>
#include <core/IEnum.h>
#include <core/IAssert.h>

#include <asset/import/builder/GltfIrBuilder.h>
#include <asset/definition/MeshDefinition.h>
#include <asset/LogCategory.h>

MK_DEFINE_DEFAULT_LOG_CATEGORY(Asset);

// Coordinate spaces:
// Gltf -> (x, right)
//      -> (y, up)
//      -> (z, out from camera)
// Mk   -> (x, right)
//      -> (y, up)
//      -> (z, into screen)

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

Result toResult(cgltf_result result)
{
    switch (result)
    {
    case cgltf_result_success:
        return Result::eOk;
    case cgltf_result_invalid_options:
        return Result::eInvalidParam;
    case cgltf_result_file_not_found:
        return Result::eNotFound;
    case cgltf_result_out_of_memory:
        return Result::eOutOfCapactiy;
    case cgltf_result_legacy_gltf:
    case cgltf_result_io_error:
    case cgltf_result_invalid_json:
    case cgltf_result_invalid_gltf:
    case cgltf_result_unknown_format:
    case cgltf_result_data_too_short:
    default:
        MK_ASSERT_UNREACHABLE();
        return Result::eUnexpected;
    }
}

mlm::mat4 getNodeMatrix(const cgltf_node& node)
{
    mlm::mat4 flipz = mlm::mat4::scale(1, 1, -1);
    mlm::mat4 transform;

    if (node.has_matrix)
    {
        transform = mlm::mat4(node.matrix[0],
                              node.matrix[1],
                              node.matrix[2],
                              node.matrix[3],
                              node.matrix[4],
                              node.matrix[5],
                              node.matrix[6],
                              node.matrix[7],
                              node.matrix[8],
                              node.matrix[9],
                              node.matrix[10],
                              node.matrix[11],
                              node.matrix[12],
                              node.matrix[13],
                              node.matrix[14],
                              node.matrix[15]);
    }

    mlm::mat4 scale = mlm::mat4::scale(1);
    mlm::mat4 translate = mlm::mat4::translate(0, 0, 0);
    mlm::mat4 rotate = mlm::mat4::rotate(mlm::quat(0, 0, 0, 1));

    if (node.has_translation)
    {
        translate =
            mlm::mat4::translate(node.translation[0], node.translation[1], node.translation[2]);
    }

    if (node.has_scale)
    {
        scale = mlm::mat4::scale(node.scale[0], node.scale[1], node.scale[2]);
    }

    if (node.has_rotation)
    {
        rotate = mlm::mat4::rotate(
            mlm::quat(node.rotation[0], node.rotation[1], node.rotation[2], node.rotation[3]));
    }

    return flipz * translate * rotate * scale * flipz;
}

// returns the collected node's index in irNodes
// NOLINTNEXTLINE(misc-no-recursion)
u16 collectNodeTree(const cgltf_data& gltf, const cgltf_node& node, Vector<ir::Node>& irNodes)
{
    MK_ASSERT(node.children_count <= UINT16_MAX);

    u16       index = irNodes.size();
    ir::Node& irNode = irNodes.push(ir::Node{});
    irNode.transform = getNodeMatrix(node);
    irNode.meshIndex = node.mesh - gltf.meshes;

    VectorView<cgltf_node*> children(node.children, node.children_count);
    for (cgltf_node*& child : children)
    {
        MK_ASSERT(child);
        irNode.children.push(collectNodeTree(gltf, *child, irNodes));
    }

    return index;
}

void collectNodes(const cgltf_data& gltf, ir::Ir& outIr)
{
    MK_ASSERT(gltf.nodes_count <= UINT16_MAX);
    VectorView<cgltf_scene> scenes;

    for (const cgltf_scene& scene : scenes)
    {
        VectorView<cgltf_node*> roots(scene.nodes, scene.nodes_count);

        for (cgltf_node*& root : roots)
        {
            MK_ASSERT(root);
            collectNodeTree(gltf, *root, outIr.nodes);
        }
    }
}

template<typename T>
void readAccessorData(const cgltf_accessor& accessor, VectorView<T> outView)
{
    MK_ASSERTF(!accessor.is_sparse, "Not implemented yet");

    const u8* src = cgltf_buffer_view_data(accessor.buffer_view);
    MK_ASSERT(src);
    src += accessor.offset;

    // componentSize: sizeof(u8), sizeof(u16) etc.
    // componentCount: vec4 -> 4, vec3 -> 3 etc.
    cgltf_size componentSize = cgltf_component_size(accessor.component_type);
    cgltf_size componentCount = cgltf_num_components(accessor.type);
    cgltf_size typeSize = componentSize * componentCount;

    MK_ASSERT(sizeof(T) >= typeSize);

    // Fast path: directly memcpyable.
    if (accessor.stride == typeSize && sizeof(T) == typeSize)
    {
        memcpy(outView.data(), src, typeSize * accessor.count);
        return;
    }

    byte* dst = asWritableBytes(outView).data();
    for (cgltf_size i = 0; i < accessor.count; i++)
    {
        memcpy(dst, src, typeSize);
        dst += typeSize;
        src += accessor.stride;
    }
}

Result collectMeshes(const cgltf_data& gltf, ir::Ir& outIr)
{
    VectorView<cgltf_mesh> meshes(gltf.meshes, gltf.meshes_count);

    for (const auto& mesh : meshes)
    {
        ir::Mesh                    irMesh;
        VectorView<cgltf_primitive> primitives(mesh.primitives, mesh.primitives_count);
        for (const auto& primitive : primitives)
        {
            ir::MeshPart    part;
            cgltf_accessor* indicesAccessor = primitive.indices;
            MK_LOG_ERROR_AND_RETURN_RET_IF_TRUE(indicesAccessor->component_type >=
                                                    cgltf_component_type_r_32u,
                                                Result::eInvalidParam,
                                                "Only supports u16 indices.");
            part.indices.resize(indicesAccessor->count);
            readAccessorData<u16>(*indicesAccessor, { part.indices.data(), part.indices.size() });

            VectorView<cgltf_attribute> attributes = { primitive.attributes,
                                                       primitive.attributes_count };
            for (const auto& attribute : attributes)
            {
                if (attribute.type == cgltf_attribute_type_position)
                {
                    part.positions.resize(attribute.data->count);
                    readAccessorData<mlm::PackedVec3>(
                        *attribute.data,
                        { part.positions.data(), part.positions.size() });
                }
            }

            for (const auto position : part.positions)
            {
                part.localBound.include(position);
            }

            irMesh.parts.push(std::move(part));
        }
        outIr.meshes.push(std::move(irMesh));
    }
    return Result::eOk;
}

} // namespace

Result GltfIrBuilder::build(ir::Ir& outIr)
{
    class CgltfDeleter
    {
    public:
        void operator()(cgltf_data* ptr)
        {
            if (ptr)
            {
                cgltf_free(ptr);
            }
        }
    };

    fs::IVfs& vfs = AppContext<fs::IVfs>::get();

    Vector<byte> bytes;
    MK_LOG_ERROR_AND_RETURN_IF_NOT_OK(fs::util::readBinaryFile(path_, bytes),
                                      "Failed to read gltf file.");

    UniquePtr<cgltf_data, CgltfDeleter> gltf;
    {
        cgltf_options options = {};
        options.memory.alloc_func = cgltfMkAllocBridge;
        options.memory.free_func = cgltfMkFreeBridge;
        options.memory.user_data = nullptr;

        cgltf_data* raw;
        MK_LOG_ERROR_AND_RETURN_IF_NOT_OK(
            toResult(cgltf_parse(&options, bytes.data(), bytes.size(), &raw)),
            "Failed to parse gltf file {}",
            path_.cstr());

        gltf = UniquePtr<cgltf_data, CgltfDeleter>(raw);

        MK_LOG_ERROR_AND_RETURN_IF_NOT_OK(
            toResult(cgltf_load_buffers(&options, raw, vfs.resolve(path_).cstr())),
            "Failed to load gltf buffers, {}",
            path_.cstr());
    }

    MK_LOG_ERROR_AND_RETURN_IF_NOT_OK(collectMeshes(*gltf, outIr),
                                      "Failed to collect meshes from: {}",
                                      path_.cstr());

    collectNodes(*gltf, outIr);

    return Result::eOk;
}

} // namespace mk::asset
