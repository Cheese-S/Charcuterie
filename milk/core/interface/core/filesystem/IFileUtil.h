#pragma once
#include <core/IResult.h>
#include <core/container/IVector.h>

namespace mk::fs
{
class Path;
}

namespace mk::fs::util
{
Result readBinaryFile(const fs::Path& path, Vector<byte>& outBytes);
Result writeBinaryFile(const fs::Path& path, VectorView<const byte> bytes);
} // namespace mk::fs::util
