#include <test/ITest.h>
#include <cfloat>

#include <asset/import/Ir.h>
#include <asset/import/builder/ITemplateAssetBuilder.h>
#include <core/log/IRawLog.h>

namespace mk::swiss::render
{
struct BvhTest;
}

#define MK_BVH_TEST_FRIEND friend struct BvhTest;
#include <swiss/render/Bvh.h>

namespace mk::swiss::render
{
struct BvhTest: ::testing::Test
{
    static bool isValid(const Bvh& bvh);

    // Test-only validators that need friend access to Bvh internals.
    static void validateRecursiveCases();
    static void validateSplitAxis();
    static void validateFindBin();

protected:
    void SetUp() override;

    UniquePtr<Bvh>  bvh_;
    mlm::Ray        hitRay_;
    mlm::PackedVec3 worldMax_;

private:
    struct TriRange
    {
        u32 start;
        u32 count;
    };

    static std::optional<TriRange>
    validateSubtree(const StackVector<Bvh::Node, 64>& nodes, usize nodeIndex, u32 totalTriCount);
};

void BvhTest::SetUp()
{
    asset::ir::Ir ir;
    ASSERT_OK(asset::TemplateAssetBuilder::build(fs::Path("../../../../../asset/bunny.glb"), ir));

    const asset::ir::MeshPart& part = ir.meshes[0].parts[0];
    mlm::PackedVec3            v0 = part.positions[part.indices[0]];
    mlm::PackedVec3            v1 = part.positions[part.indices[1]];
    mlm::PackedVec3            v2 = part.positions[part.indices[2]];

    mlm::PackedVec3 localCentroid = (v0 + v1 + v2) / 3.0f;
    mlm::PackedVec3 localNormal = mlm::normalize(mlm::cross(v1 - v0, v2 - v0));

    mlm::mat4 worldFromLocal = ir.entities[0].transform;

    // World-space ray that hits the first triangle: offset one local unit along
    // the triangle normal, pointing back at the surface.
    mlm::Ray localHitRay{ .o = localCentroid + localNormal, .d = localNormal / -1.0f };
    hitRay_ = mlm::transformRay(worldFromLocal, localHitRay);

    // World-space AABB of the mesh, used to place a guaranteed miss ray.
    mlm::PackedVec3 lo = part.localBound.min();
    mlm::PackedVec3 hi = part.localBound.max();
    mlm::Bound      worldBound;
    for (u32 i = 0; i < 8; i++)
    {
        mlm::PackedVec3 corner((i & 1) ? hi.x() : lo.x(),
                               (i & 2) ? hi.y() : lo.y(),
                               (i & 4) ? hi.z() : lo.z());
        worldBound.toInclude(mlm::transformPoint(worldFromLocal, corner));
    }
    worldMax_ = worldBound.max();

    bvh_ = makeUnique<Bvh>(std::move(ir));
}

bool BvhTest::isValid(const Bvh& bvh)
{
    if (bvh.nodes_.empty())
    {
        return false;
    }

    u32                     totalTriCount = static_cast<u32>(bvh.tris_.size());
    std::optional<TriRange> range = validateSubtree(bvh.nodes_, 0, totalTriCount);
    return range.has_value() && range->start == 0 && range->count == totalTriCount;
}

// NOLINTNEXTLINE(misc-no-recursion)
std::optional<BvhTest::TriRange> BvhTest::validateSubtree(const StackVector<Bvh::Node, 64>& nodes,
                                                          usize nodeIndex,
                                                          u32   totalTriCount)
{
    if (nodeIndex >= nodes.size())
    {
        return std::nullopt;
    }

    const Bvh::Node& node = nodes[nodeIndex];

    if (node.triCount != 0) // leaf
    {
        MK_RAW_LOG_DEBUG("node[{}] leaf: triStart={} triCount={} axis={}",
                         nodeIndex,
                         node.triStart,
                         node.triCount,
                         static_cast<u32>(node.axis));

        if (node.triStart > totalTriCount - node.triCount)
        {
            return std::nullopt;
        }
        return TriRange{ .start = node.triStart, .count = node.triCount };
    }

    // internal node
    usize leftIndex = nodeIndex + 1;
    usize rightIndex = node.rightChildIndex;

    if (leftIndex >= nodes.size() || rightIndex <= leftIndex || rightIndex >= nodes.size())
    {
        return std::nullopt;
    }

    const Bvh::Node& left = nodes[leftIndex];
    const Bvh::Node& right = nodes[rightIndex];

    if (!node.localBound.doesInclude(left.localBound) ||
        !node.localBound.doesInclude(right.localBound))
    {
        return std::nullopt;
    }

    std::optional<TriRange> leftRange = validateSubtree(nodes, leftIndex, totalTriCount);
    std::optional<TriRange> rightRange = validateSubtree(nodes, rightIndex, totalTriCount);
    if (!leftRange || !rightRange)
    {
        return std::nullopt;
    }

    if (rightRange->start != leftRange->start + leftRange->count)
    {
        return std::nullopt;
    }

    return TriRange{ .start = leftRange->start, .count = leftRange->count + rightRange->count };
}

void BvhTest::validateRecursiveCases()
{
    mlm::Bound leafBound(mlm::PackedVec3(0.f, 0.f, 0.f), mlm::PackedVec3(1.f, 1.f, 1.f));
    mlm::Bound rightBound(mlm::PackedVec3(2.f, 2.f, 2.f), mlm::PackedVec3(3.f, 3.f, 3.f));
    mlm::Bound parentBound(mlm::PackedVec3(0.f, 0.f, 0.f), mlm::PackedVec3(3.f, 3.f, 3.f));
    mlm::Bound outsideBound(mlm::PackedVec3(5.f, 5.f, 5.f), mlm::PackedVec3(6.f, 6.f, 6.f));

    auto makeLeaf = [](u32 triStart, u16 triCount, mlm::Bound bound)
    {
        return Bvh::Node{ .localBound = bound,
                          .triStart = triStart,
                          .triCount = triCount,
                          .axis = Bvh::Axis::eX };
    };

    auto makeInternal = [](u32 rightChildIndex, mlm::Bound bound)
    {
        return Bvh::Node{ .localBound = bound,
                          .rightChildIndex = rightChildIndex,
                          .triCount = 0,
                          .axis = Bvh::Axis::eX };
    };

    // Leaf is always valid.
    {
        StackVector<Bvh::Node, 64> nodes;
        nodes.push(makeLeaf(0, 1, leafBound));
        std::optional<TriRange> range = validateSubtree(nodes, 0, 5);
        ASSERT_TRUE(range.has_value());
        EXPECT_EQ(range->start, 0u);
        EXPECT_EQ(range->count, 1u);
    }

    // Leaf whose range exceeds the tri array is invalid.
    {
        StackVector<Bvh::Node, 64> nodes;
        nodes.push(makeLeaf(4, 2, leafBound));
        EXPECT_FALSE(validateSubtree(nodes, 0, 5).has_value());
    }

    // Internal node with continuous children.
    {
        StackVector<Bvh::Node, 64> nodes;
        nodes.push(makeInternal(2, parentBound));
        nodes.push(makeLeaf(0, 2, leafBound));
        nodes.push(makeLeaf(2, 3, rightBound));
        std::optional<TriRange> range = validateSubtree(nodes, 0, 5);
        ASSERT_TRUE(range.has_value());
        EXPECT_EQ(range->start, 0u);
        EXPECT_EQ(range->count, 5u);
    }

    // Children must cover a continuous tri range.
    {
        StackVector<Bvh::Node, 64> nodes;
        nodes.push(makeInternal(2, parentBound));
        nodes.push(makeLeaf(0, 2, leafBound));
        nodes.push(makeLeaf(3, 2, rightBound));
        EXPECT_FALSE(validateSubtree(nodes, 0, 5).has_value());
    }

    // Child bound must be contained by the parent's.
    {
        StackVector<Bvh::Node, 64> nodes;
        nodes.push(makeInternal(2, parentBound));
        nodes.push(makeLeaf(0, 2, outsideBound));
        nodes.push(makeLeaf(2, 3, rightBound));
        EXPECT_FALSE(validateSubtree(nodes, 0, 5).has_value());
    }

    // rightChildIndex out of range.
    {
        StackVector<Bvh::Node, 64> nodes;
        nodes.push(makeInternal(99, parentBound));
        nodes.push(makeLeaf(0, 2, leafBound));
        EXPECT_FALSE(validateSubtree(nodes, 0, 5).has_value());
    }
}

void BvhTest::validateSplitAxis()
{
    EXPECT_EQ(Bvh::chooseSplitAxis(
                  mlm::Bound(mlm::PackedVec3(0.f, 0.f, 0.f), mlm::PackedVec3(3.f, 1.f, 1.f))),
              Bvh::Axis::eX);
    EXPECT_EQ(Bvh::chooseSplitAxis(
                  mlm::Bound(mlm::PackedVec3(0.f, 0.f, 0.f), mlm::PackedVec3(1.f, 3.f, 1.f))),
              Bvh::Axis::eY);
    EXPECT_EQ(Bvh::chooseSplitAxis(
                  mlm::Bound(mlm::PackedVec3(0.f, 0.f, 0.f), mlm::PackedVec3(1.f, 1.f, 3.f))),
              Bvh::Axis::eZ);
}

void BvhTest::validateFindBin()
{
    mlm::Bound bound(mlm::PackedVec3(0.f, 0.f, 0.f), mlm::PackedVec3(12.f, 0.f, 0.f));

    Bvh::SahTri atMin = { .indexStart = 0, .centeroid = mlm::PackedVec3(0.f, 0.f, 0.f), .bound = {} };
    Bvh::SahTri atMid = { .indexStart = 0, .centeroid = mlm::PackedVec3(6.f, 0.f, 0.f), .bound = {} };
    Bvh::SahTri atMax = { .indexStart = 0, .centeroid = mlm::PackedVec3(12.f, 0.f, 0.f), .bound = {} };

    EXPECT_EQ(Bvh::findBin(bound, atMin, Bvh::Axis::eX), 0);
    EXPECT_EQ(Bvh::findBin(bound, atMid, Bvh::Axis::eX), 6);
    EXPECT_EQ(Bvh::findBin(bound, atMax, Bvh::Axis::eX), 11);
}

// ------------------------------- Validation ----------------------------------

TEST_F(BvhTest, IsValidOnBunny)
{
    EXPECT_TRUE(isValid(*bvh_));
}

// ------------------------------ Intersection ---------------------------------

TEST_F(BvhTest, HitsTriangleCentroid)
{
    EXPECT_TRUE(bvh_->intersect(hitRay_, FLT_MAX));
}

TEST_F(BvhTest, MissFarOutside)
{
    mlm::Ray r{ .o = worldMax_ + mlm::PackedVec3(0.f, 0.f, 10.f),
                .d = mlm::PackedVec3(0.f, 0.f, 1.f) };
    EXPECT_FALSE(bvh_->intersect(r, FLT_MAX));
}

TEST_F(BvhTest, RespectsTMax)
{
    EXPECT_FALSE(bvh_->intersect(hitRay_, 0.1f));
    EXPECT_TRUE(bvh_->intersect(hitRay_, 10.0f));
}

// ----------------------------- Private helpers --------------------------------

TEST(BvhHelpers, IsValidRecursiveCases)
{
    BvhTest::validateRecursiveCases();
}

TEST(BvhHelpers, ChooseSplitAxis)
{
    BvhTest::validateSplitAxis();
}

TEST(BvhHelpers, FindBin)
{
    BvhTest::validateFindBin();
}

} // namespace mk::swiss::render
MK_FULL_MAIN()
