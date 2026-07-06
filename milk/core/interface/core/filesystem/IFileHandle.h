#pragma once
#include <core/IType.h>
#include <core/IResult.h>
#include <core/fwd/IStringFwd.h>
#include <core/fwd/IVectorFwd.h>

namespace mk::fs
{
class Path;
class IFileHandle
{
public:
    virtual ~IFileHandle() = default;

    [[nodiscard]] virtual Result read(Vector<byte>& dst) = 0;
    [[nodiscard]] virtual Result read(String& dst) = 0;
    [[nodiscard]] virtual Result read(void* dst, usize dstSize) = 0;

    [[nodiscard]] virtual Result write(VectorView<const byte> data) = 0;
    [[nodiscard]] virtual Result write(const void* buf, usize size) = 0;
    [[nodiscard]] virtual Result write(StringView str) = 0;
    virtual usize                size() = 0;
};
} // namespace mk::fs
