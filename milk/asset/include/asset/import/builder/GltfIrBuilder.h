#pragma once
#include <core/filesystem/IPath.h>
#include <asset/import/Ir.h>

namespace mk::asset
{
class GltfIrBuilder
{
public:
    GltfIrBuilder(fs::Path&& path): path_(std::move(path)) {}
    MK_NON_MOVABLE_NON_COPYABLE(GltfIrBuilder);

    Result build(ir::Ir& outIr);

private:
    fs::Path path_;
};
} // namespace mk::asset
