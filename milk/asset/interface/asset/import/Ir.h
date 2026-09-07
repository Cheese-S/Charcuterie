#pragma once
#include <core/mlm/IBound.h>
#include <core/container/IVector.h>

namespace mk::asset::ir
{

struct Entity
{
    mlm::mat4   transform;
    u16         meshIndex;
    Vector<u16> children;
};

struct MeshPart
{
    Vector<mlm::PackedVec3> positions;
    // indices.size() <= kU32Max
    Vector<u16>             indices;
    mlm::Bound              localBound;
};

struct Mesh
{
    Vector<MeshPart> parts;
};

struct Ir
{
    Vector<Entity> entities;
    Vector<Mesh>   meshes;
};
} // namespace mk::asset::ir
