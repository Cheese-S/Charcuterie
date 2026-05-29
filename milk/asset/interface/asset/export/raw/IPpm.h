#pragma once
#include <core/IType.h>
#include <core/filesystem/IPath.h>

namespace mk::asset::exp
{
Result savePpm(const fs::Path& path, VectorView<float> data, u16 width, u16 height);
}
