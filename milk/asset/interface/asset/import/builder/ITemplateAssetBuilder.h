#pragma once
#include <core/filesystem/IPath.h>
#include <asset/definition/TemplateDefinition.h>

namespace mk::asset
{

class TemplateAssetBuilder
{
public:
    MK_NON_MOVABLE_NON_COPYABLE(TemplateAssetBuilder);

    // TODO(Cheese_S): obviously not correct. should build template definition instead
    static Result build(const fs::Path& path, ir::Ir& outIr);
};

} // namespace mk::asset
