#include <core/container/IVector.h>
#include <core/IMacro.h>

namespace mk::io
{

class BufferedWriter
{
public:
    MK_NON_MOVABLE_NON_COPYABLE(BufferedWriter);

    BufferedWriter(): head_(0) {}

    void operator<<(u8 val);
    void operator<<(u16 val);
    void operator<<(u32 val);
    void operator<<(u64 val);

    void operator<<(i8 val);
    void operator<<(i16 val);
    void operator<<(i32 val);
    void operator<<(i64 val);

    void operator<<(f32 val);
    void operator<<(f64 val);

    void operator<<(StringView val);

    VectorView<byte> getView();

private:
    void serialize(const void* data, usize size);

    Vector<byte> bytes_;
    usize        head_;
};

} // namespace mk::io
