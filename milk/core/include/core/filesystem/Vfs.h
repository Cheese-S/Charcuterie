#pragma once
#include <core/IResult.h>
#include <core/filesystem/IPath.h>

namespace mk::fs
{
Result getExePath(Path& outPath);
}
