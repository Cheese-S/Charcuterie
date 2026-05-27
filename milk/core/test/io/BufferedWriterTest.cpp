#include <gtest/gtest.h>
#include <core/io/IBufferedWriter.h>
#include <test/ITest.h>
#include <cstring>

namespace mk::io
{

template<typename T>
static T readAs(const byte* ptr)
{
    T val;
    std::memcpy(&val, ptr, sizeof(T));
    return val;
}

class BufferedWriterTest: public MilkTest
{
protected:
    BufferedWriter writer_;
};

// ---------------------------------------------------------------------------
// Single-type round-trips
// ---------------------------------------------------------------------------

TEST_F(BufferedWriterTest, WriteU8)
{
    writer_ << static_cast<u8>(0xAB);

    auto view = writer_.getView();
    ASSERT_EQ(view.size(), 1U);
    EXPECT_EQ(view[0], static_cast<byte>(0xAB));
}

TEST_F(BufferedWriterTest, WriteU16)
{
    writer_ << static_cast<u16>(0x1234);

    auto view = writer_.getView();
    ASSERT_EQ(view.size(), 2U);
    EXPECT_EQ(readAs<u16>(view.data()), static_cast<u16>(0x1234));
}

TEST_F(BufferedWriterTest, WriteU32)
{
    writer_ << static_cast<u32>(0xDEADBEEF);

    auto view = writer_.getView();
    ASSERT_EQ(view.size(), 4U);
    EXPECT_EQ(readAs<u32>(view.data()), static_cast<u32>(0xDEADBEEF));
}

TEST_F(BufferedWriterTest, WriteU64)
{
    writer_ << static_cast<u64>(0xCAFEBABEDEADBEEF);

    auto view = writer_.getView();
    ASSERT_EQ(view.size(), 8U);
    EXPECT_EQ(readAs<u64>(view.data()), static_cast<u64>(0xCAFEBABEDEADBEEF));
}

TEST_F(BufferedWriterTest, WriteI8)
{
    writer_ << static_cast<i8>(-1);

    auto view = writer_.getView();
    ASSERT_EQ(view.size(), 1U);
    EXPECT_EQ(readAs<i8>(view.data()), static_cast<i8>(-1));
}

TEST_F(BufferedWriterTest, WriteI16)
{
    writer_ << static_cast<i16>(-1000);

    auto view = writer_.getView();
    ASSERT_EQ(view.size(), 2U);
    EXPECT_EQ(readAs<i16>(view.data()), static_cast<i16>(-1000));
}

TEST_F(BufferedWriterTest, WriteI32)
{
    writer_ << static_cast<i32>(-123456789);

    auto view = writer_.getView();
    ASSERT_EQ(view.size(), 4U);
    EXPECT_EQ(readAs<i32>(view.data()), static_cast<i32>(-123456789));
}

TEST_F(BufferedWriterTest, WriteI64)
{
    writer_ << static_cast<i64>(-9000000000000000000LL);

    auto view = writer_.getView();
    ASSERT_EQ(view.size(), 8U);
    EXPECT_EQ(readAs<i64>(view.data()), static_cast<i64>(-9000000000000000000LL));
}

TEST_F(BufferedWriterTest, WriteF32)
{
    static constexpr f32 kValue = 3.14159F;
    writer_ << kValue;

    auto view = writer_.getView();
    ASSERT_EQ(view.size(), 4U);
    EXPECT_EQ(readAs<f32>(view.data()), kValue);
}

TEST_F(BufferedWriterTest, WriteF64)
{
    static constexpr f64 kValue = 2.718281828459045;
    writer_ << kValue;

    auto view = writer_.getView();
    ASSERT_EQ(view.size(), 8U);
    EXPECT_EQ(readAs<f64>(view.data()), kValue);
}

TEST_F(BufferedWriterTest, WriteString)
{
    writer_ << StringView("hello");

    auto view = writer_.getView();
    ASSERT_EQ(view.size(), 5U);
    EXPECT_EQ(view[0], static_cast<byte>('h'));
    EXPECT_EQ(view[1], static_cast<byte>('e'));
    EXPECT_EQ(view[2], static_cast<byte>('l'));
    EXPECT_EQ(view[3], static_cast<byte>('l'));
    EXPECT_EQ(view[4], static_cast<byte>('o'));
}

TEST_F(BufferedWriterTest, WriteEmptyString)
{
    writer_ << StringView("");

    EXPECT_EQ(writer_.getView().size(), 0U);
}

// ---------------------------------------------------------------------------
// Accumulation and layout
// ---------------------------------------------------------------------------

TEST_F(BufferedWriterTest, SequentialWritesAccumulate)
{
    writer_ << static_cast<u8>(0x01);
    writer_ << static_cast<u8>(0x02);
    writer_ << static_cast<u8>(0x03);

    auto view = writer_.getView();
    ASSERT_EQ(view.size(), 3U);
    EXPECT_EQ(view[0], static_cast<byte>(0x01));
    EXPECT_EQ(view[1], static_cast<byte>(0x02));
    EXPECT_EQ(view[2], static_cast<byte>(0x03));
}

TEST_F(BufferedWriterTest, MixedWritesPackedContiguously)
{
    // Layout (little-endian):
    //   [0]      u8  = 0xFF           (1 byte)
    //   [1..2]   u16 = 0x0102        (2 bytes)
    //   [3..6]   u32 = 0xAABBCCDD    (4 bytes)
    //   [7..8]   str = "hi"          (2 bytes)
    //   total: 9 bytes

    writer_ << static_cast<u8>(0xFF);
    writer_ << static_cast<u16>(0x0102);
    writer_ << static_cast<u32>(0xAABBCCDD);
    writer_ << StringView("hi");

    auto view = writer_.getView();
    ASSERT_EQ(view.size(), 9U);

    EXPECT_EQ(view[0], static_cast<byte>(0xFF));
    EXPECT_EQ(readAs<u16>(view.data() + 1), static_cast<u16>(0x0102));
    EXPECT_EQ(readAs<u32>(view.data() + 3), static_cast<u32>(0xAABBCCDD));
    EXPECT_EQ(view[7], static_cast<byte>('h'));
    EXPECT_EQ(view[8], static_cast<byte>('i'));
}

TEST_F(BufferedWriterTest, GetViewReflectsSizeAfterEachWrite)
{
    writer_ << static_cast<u8>(0xAA);
    EXPECT_EQ(writer_.getView().size(), 1U);

    writer_ << static_cast<u32>(0x11223344);
    EXPECT_EQ(writer_.getView().size(), 5U);

    writer_ << StringView("abc");
    EXPECT_EQ(writer_.getView().size(), 8U);
}

TEST_F(BufferedWriterTest, BoundaryValues)
{
    writer_ << static_cast<u8>(0x00);
    writer_ << static_cast<u8>(0xFF);
    writer_ << static_cast<i8>(0);
    writer_ << static_cast<i8>(127);
    writer_ << static_cast<i8>(-128);
    writer_ << 0.0F;
    writer_ << 0.0;

    static constexpr usize kExpectedSize = 1U + 1U + 1U + 1U + 1U + 4U + 8U;

    auto view = writer_.getView();
    ASSERT_EQ(view.size(), kExpectedSize);

    EXPECT_EQ(view[0], static_cast<byte>(0x00));
    EXPECT_EQ(view[1], static_cast<byte>(0xFF));
    EXPECT_EQ(readAs<i8>(view.data() + 2), static_cast<i8>(0));
    EXPECT_EQ(readAs<i8>(view.data() + 3), static_cast<i8>(127));
    EXPECT_EQ(readAs<i8>(view.data() + 4), static_cast<i8>(-128));
    EXPECT_EQ(readAs<f32>(view.data() + 5), 0.0F);
    EXPECT_EQ(readAs<f64>(view.data() + 9), 0.0);
}

} // namespace mk::io
