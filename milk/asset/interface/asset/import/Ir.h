#pragma once
#include <core/mlm/IUtil.h>
#include <core/container/IVector.h>

namespace mk::asset::ir
{

struct Node
{
    mlm::mat4   transform;
    u16         meshIndex;
    Vector<u16> children;
};

struct MeshPart
{
    Vector<mlm::PackedVec3> positions;
    Vector<u16>             indices;
    mlm::Bound              localBound;
};

struct Mesh
{
    Vector<MeshPart> parts;
};

struct Ir
{
    Vector<Node> nodes;
    Vector<Mesh> meshes;
};
} // namespace mk::asset::ir
