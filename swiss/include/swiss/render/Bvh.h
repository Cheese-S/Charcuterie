#pragma once

#include <asset/import/Ir.h>

namespace mk::swiss::render
{
class Bvh
{
    struct Node
    {
        Node*      left;
        Node*      right;
        mlm::Bound worldBound;
        u16        triStart;
        u16        triCount;
    };

    struct Tri
    {
        u16             indexStart;
        mlm::PackedVec3 centeroid;
    };

    enum Axis
    {
        eX = 0,
        eY = 1,
        eZ = 2
    };

public:
    Bvh(asset::ir::Ir&& ir);
    bool intersect();

private:
    void buildPart(const asset::ir::MeshPart& part);
    void split(Bvh::Node& node);
    void chooseSplitAxis();
    f32  sah();

    asset::ir::Ir         ir_;
    StackVector<Node, 64> nodes_;
};
} // namespace mk::swiss::render
