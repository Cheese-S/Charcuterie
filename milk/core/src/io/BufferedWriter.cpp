#include <core/io/IBufferedWriter.h>
#include <core/container/IString.h>

namespace mk::io
{
void BufferedWriter::serialize(const void* data, usize size)
{
    if (head_ + size > bytes_.size())
    {
        bytes_.resize(head_ + size);
    }

    memcpy(bytes_.begin() + head_, data, size);
    head_ += size;
}

void BufferedWriter::operator<<(u8 val)
{
    serialize(&val, 1);
}

void BufferedWriter::operator<<(u16 val)
{
    serialize(&val, 2);
}

void BufferedWriter::operator<<(u32 val)
{
    serialize(&val, 4);
}

void BufferedWriter::operator<<(u64 val)
{
    serialize(&val, 8);
}

void BufferedWriter::operator<<(i8 val)
{
    serialize(&val, 1);
}

void BufferedWriter::operator<<(i16 val)
{
    serialize(&val, 2);
}

void BufferedWriter::operator<<(i32 val)
{
    serialize(&val, 4);
}

void BufferedWriter::operator<<(i64 val)
{
    serialize(&val, 8);
}

void BufferedWriter::operator<<(f32 val)
{
    serialize(&val, 4);
}

void BufferedWriter::operator<<(f64 val)
{
    serialize(&val, 8);
}

void BufferedWriter::operator<<(StringView val)
{
    serialize(val.data(), val.size());
}

VectorView<byte> BufferedWriter::getView()
{
    return { bytes_.begin(), head_ };
}

} // namespace mk::io
