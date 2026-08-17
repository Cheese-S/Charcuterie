#include <test/ITest.h>

#include <cstring>

#include <core/filesystem/IFileUtil.h>
#include <core/container/IString.h>
#include <asset/import/Ir.h>
#include <asset/import/builder/GltfIrBuilder.h>

namespace mk::asset
{

class GltfIrBuilderTest: public ::testing::Test
{
public:
    GltfIrBuilderTest(): path_("triangle.gltf") {};

protected:
    void SetUp() override {}

    void writeTextFile(StringView text)
    {
        Vector<byte> bytes;
        bytes.resize(text.size());
        memcpy(bytes.data(), text.data(), text.size());
        EXPECT_OK(fs::util::writeBinaryFile(path_, { bytes.data(), bytes.size() }));
    }

    void TearDown() override
    {
        fs::IVfs& vfs = AppContext<fs::IVfs>::get();
        if (vfs.fileExist(path_))
        {
            EXPECT_OK(vfs.deleteFile(path_));
        }
    }

    fs::Path path_;
};

TEST_F(GltfIrBuilderTest, BuildsTriangle)
{
    // Single triangle:
    // positions: (1,2,3), (4,5,6), (7,8,9)
    // indices:   0, 1, 2
    // Embedded base64 buffer, 42 bytes: 36 bytes positions + 6 bytes indices.
    writeTextFile(R"({
  "asset": { "version": "2.0" },
  "scene": 0,
  "scenes": [ { "nodes": [0] } ],
  "nodes": [ { "mesh": 0 } ],
  "meshes": [
    {
      "primitives": [
        { "attributes": { "POSITION": 0 }, "indices": 1 }
      ]
    }
  ],
  "buffers": [
    {
      "byteLength": 42,
      "uri": "data:application/octet-stream;base64,AACAPwAAAEAAAEBAAACAQAAAoEAAAMBAAADgQAAAAEEAABBBAAABAAIA"
    }
  ],
  "bufferViews": [
    { "buffer": 0, "byteOffset": 0, "byteLength": 36 },
    { "buffer": 0, "byteOffset": 36, "byteLength": 6 }
  ],
  "accessors": [
    { "bufferView": 0, "componentType": 5126, "count": 3, "type": "VEC3" },
    { "bufferView": 1, "componentType": 5123, "count": 3, "type": "SCALAR" }
  ]
})");

    ir::Ir        ir;
    GltfIrBuilder builder(fs::Path{ path_ });
    EXPECT_OK(builder.build(ir));

    ASSERT_EQ(ir.meshes.size(), 1);
    ASSERT_EQ(ir.meshes[0].parts.size(), 1);
    const ir::MeshPart& part = ir.meshes[0].parts[0];

    // Z is flipped on import.
    ASSERT_EQ(part.positions.size(), 3);
    EXPECT_FLOAT_EQ(part.positions[0].x(), 1.0F);
    EXPECT_FLOAT_EQ(part.positions[0].y(), 2.0F);
    EXPECT_FLOAT_EQ(part.positions[0].z(), -3.0F);
    EXPECT_FLOAT_EQ(part.positions[1].x(), 4.0F);
    EXPECT_FLOAT_EQ(part.positions[1].y(), 5.0F);
    EXPECT_FLOAT_EQ(part.positions[1].z(), -6.0F);
    EXPECT_FLOAT_EQ(part.positions[2].x(), 7.0F);
    EXPECT_FLOAT_EQ(part.positions[2].y(), 8.0F);
    EXPECT_FLOAT_EQ(part.positions[2].z(), -9.0F);

    // Winding order is swapped (1 <-> 2).
    ASSERT_EQ(part.indices.size(), 3);
    EXPECT_EQ(part.indices[0], 0);
    EXPECT_EQ(part.indices[1], 2);
    EXPECT_EQ(part.indices[2], 1);

    EXPECT_FLOAT_EQ(part.localBound.min().x(), 1.0F);
    EXPECT_FLOAT_EQ(part.localBound.min().y(), 2.0F);
    EXPECT_FLOAT_EQ(part.localBound.min().z(), -9.0F);
    EXPECT_FLOAT_EQ(part.localBound.max().x(), 7.0F);
    EXPECT_FLOAT_EQ(part.localBound.max().y(), 8.0F);
    EXPECT_FLOAT_EQ(part.localBound.max().z(), -3.0F);

    ASSERT_EQ(ir.nodes.size(), 1);
    EXPECT_EQ(ir.nodes[0].meshIndex, 0);
    EXPECT_TRUE(ir.nodes[0].children.empty());
    EXPECT_EQ(ir.nodes[0].transform, mlm::mat4());
}

TEST_F(GltfIrBuilderTest, MissingFile)
{
    ir::Ir        ir;
    GltfIrBuilder builder(fs::Path{ "does_not_exist.gltf" });
    EXPECT_NOT_OK(builder.build(ir));
}

} // namespace mk::asset
MK_FULL_MAIN()
