#pragma once

#include <asset/import/Ir.h>
#include <core/mlm/IRay.h>
#include <optional>

#ifndef MK_BVH_TEST_FRIEND
    #define MK_BVH_TEST_FRIEND
#endif

// TODO(Cheese_S): optimize + figure out how we can accomdate BLAS and TLAS
namespace mk::swiss::render
{
class Bvh
{
    struct SahTri
    {
        u32             indexStart;
        mlm::PackedVec3 centeroid;
        mlm::Bound      bound;
    };

    struct SahBin
    {
        u16        count;
        mlm::Bound bound;
    };

    enum Axis : u8
    {
        eX = 0,
        eY = 1,
        eZ = 2
    };

    // Internal node: triCount == 0, has rightChildOffset.
    // Leaf node    : triCount != 0, has triStart. Axis is meaningless
    struct Node
    {
        mlm::Bound localBound;
        // Internal node: has rightChildOffset.
        // Leaf node: has triStart.
        union
        {
            u32 triStart;
            u32 rightChildIndex;
        };

        u16  triCount;
        Axis axis;
    };

    struct SplitResult
    {
        u32  rightChildIndex;
        Axis axis;
    };

    static_assert(sizeof(Node) == 32);

    static constexpr u8 kNumSahBins = 12;

    MK_BVH_TEST_FRIEND

public:
    Bvh(asset::ir::Ir&& ir);
    bool intersect(const mlm::Ray& r, f32 tMax) const;

private:
    void buildPart(const asset::ir::MeshPart& part);

    [[nodiscard]] std::optional<SplitResult>
    split(Vector<SahTri>& tris, u32 parentTriStart, u16 parentTriCount);

    // helpers
    static Axis chooseSplitAxis(const mlm::Bound& bound);
    static u8   findBin(const mlm::Bound& bound, const SahTri& tri, Axis axis);
    static void
    setSplitResult(Node& node, std::optional<SplitResult> result, u32 triStart, u16 triCount);

    asset::ir::Ir         ir_;
    mlm::mat4             worldToLocal_;
    // tri contain the first index into the indices array
    // usage: triangle index = indices_[tris_[0] + 0]
    Vector<u32>           tris_;
    StackVector<Node, 64> nodes_;
};

#ifdef MK_BVH_TEST_FRIEND
    #undef MK_BVH_TEST_FRIEND
#endif

} // namespace mk::swiss::render
