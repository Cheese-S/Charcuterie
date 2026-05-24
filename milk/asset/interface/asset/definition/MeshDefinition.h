#pragma once

#include <core/container/IVector.h>

namespace mk::asset
{
// MK uses left handed y-up coordinate system
// +x right, +y up, +z forward
struct MeshDefinition
{
    enum class VertexStreamType
    {
        ePosition,
    };

    enum class VertexStreamFormat
    {
        eVec4,
    };
};
} // namespace mk::asset
