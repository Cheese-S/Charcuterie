#pragma once
#include <core/filesystem/IPath.h>
#include <asset/definition/MeshDefinition.h>

namespace mk::asset
{
Result importGltf(const fs::Path& path, MeshDefinition& outDefinition);
}
