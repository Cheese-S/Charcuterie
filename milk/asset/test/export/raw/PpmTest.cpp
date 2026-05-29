#include <test/ITest.h>
#include <core/filesystem/IFile.h>
#include <asset/export/raw/IPpm.h>

namespace mk::asset
{
class PpmTest: public ::testing::Test
{
public:
    PpmTest(): path_("test.ppm") {};

protected:
    void SetUp() override {}

    Vector<byte> readTestPpmBinary()
    {
        fs::IFileHandlePtr file;
        EXPECT_OK(fs::openUniqueFile(path_, fs::FileAccessMode::eRead, file));
        Vector<byte> bytes;
        EXPECT_OK(file->read(bytes));
        return bytes;
    }

    void TearDown() override
    {
        EXPECT_OK(fs::removeFile(path_));
    }

    fs::Path path_;
};

TEST_F(PpmTest, WritesCorrectContent)
{
    // 2x1: known colors — red and blue
    float data[] = {
        1.0F, 0.0F, 0.0F, // pixel 0: red
        0.0F, 0.0F, 1.0F, // pixel 1: blue
    };
    EXPECT_OK(exp::savePpm(path_, VectorView<float>(data), 2, 1));

    auto bytes = readTestPpmBinary();

    // Header
    StackString<64> expectedHeader;
    expectedHeader.fmt("P6\n{} {}\n255\n", 2, 1);
    size_t hSize = expectedHeader.size();

    // Total size
    EXPECT_EQ(bytes.size(), hSize + 2 * 1 * 3);

    // Header content
    for (size_t i = 0; i < hSize; ++i)
    {
        EXPECT_EQ(static_cast<char>(bytes[i]), expectedHeader[i]);
    }

    // Pixel data
    EXPECT_EQ(static_cast<uint8_t>(bytes[hSize + 0]), 255); // pixel 0 R
    EXPECT_EQ(static_cast<uint8_t>(bytes[hSize + 1]), 0);   // pixel 0 G
    EXPECT_EQ(static_cast<uint8_t>(bytes[hSize + 2]), 0);   // pixel 0 B
    EXPECT_EQ(static_cast<uint8_t>(bytes[hSize + 3]), 0);   // pixel 1 R
    EXPECT_EQ(static_cast<uint8_t>(bytes[hSize + 4]), 0);   // pixel 1 G
    EXPECT_EQ(static_cast<uint8_t>(bytes[hSize + 5]), 255); // pixel 1 B
}

} // namespace mk::asset
