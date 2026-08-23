#pragma once

#include <asset/import/Ir.h>

#ifndef MK_BVH_TEST_FRIEND
    #define MK_BVH_TEST_FRIEND
#endif

// TODO(Cheese_S): optimize + figure out how we can accomdate BLAS and TLAS
namespace mk::swiss::render
{
class Bvh
{
    struct Node
    {
        Node*      left;
        Node*      right;
        mlm::Bound localBound;
        u32        triStart;
        u32        triCount;
    };

    struct Tri
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

    enum Axis
    {
        eX = 0,
        eY = 1,
        eZ = 2
    };

    static constexpr u8 kNumSahBins = 12;

    MK_BVH_TEST_FRIEND
public:
    Bvh(asset::ir::Ir&& ir);
    bool intersect(const mlm::Ray& r, f32 tMax) const;

private:
    static Axis chooseSplitAxis(const mlm::Bound& bound);
    static u8   findBin(const mlm::Bound& bound, const Tri& tri, Axis axis);

    void buildPart(const asset::ir::MeshPart& part);
    void split(Bvh::Node& parent);

    asset::ir::Ir         ir_;
    mlm::mat4             worldToLocal_;
    Vector<Tri>           tris_;
    StackVector<Node, 64> nodes_;
};

#ifdef MK_BVH_TEST_FRIEND
    #undef MK_BVH_TEST_FRIEND
#endif

} // namespace mk::swiss::render
