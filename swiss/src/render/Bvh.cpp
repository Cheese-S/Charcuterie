#include <swiss/render/Bvh.h>

namespace mk::swiss::render
{
Bvh::Bvh(asset::ir::Ir&& ir): ir_(std::move(ir)) {}

void Bvh::buildPart(const asset::ir::MeshPart& part)
{
    MK_ASSERT((part.indices.size() / 3) == 0);

    Vector<Tri> tris;
    tris.reserve(part.indices.size() / 3);

    // We only support u16 as the indices type anyway
    // NOLINTNEXTLINE(bugprone-too-small-loop-variable)
    for (u16 i = 0; i < part.indices.size(); i += 3)
    {
        mlm::PackedVec3 v0 = part.positions[part.indices[i]];
        mlm::PackedVec3 v1 = part.positions[part.indices[i + 1]];
        mlm::PackedVec3 v2 = part.positions[part.indices[i + 2]];

        tris.push({
            .indexStart = i,
            .centeroid = (v0 + v1 + v2) / 3.0f,
        });
    }

    [[maybe_unused]] Bvh::Node root = { .left = nullptr,
                                        .right = nullptr,
                                        .worldBound = part.localBound,
                                        .triStart = 0,
                                        .triCount = static_cast<u16>(part.indices.size() / 3) };
}

void Bvh::split([[maybe_unused]] Bvh::Node& node) {}

// bool Bvh::intersect()
// {
//     for (const auto& mesh : ir_.meshes)
//     {
//         for (const auto& part : mesh.parts)
//         {
//         }
//     }
// }
} // namespace mk::swiss::render
