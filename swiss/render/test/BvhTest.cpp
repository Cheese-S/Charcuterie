#include <test/ITest.h>
#include <cfloat>

#include <asset/import/Ir.h>
#include <asset/import/builder/ITemplateAssetBuilder.h>

namespace mk::swiss::render
{
struct BvhTest;
}

#define MK_BVH_TEST_FRIEND friend struct BvhTest;
#include <swiss/render/Bvh.h>

namespace mk::swiss::render
{
struct BvhTest : ::testing::Test
{
    static bool isValid(const Bvh& bvh);

    // Test-only validators that need friend access to Bvh internals.
    static void validateRecursiveCases();
    static void validateSplitAxis();
    static void validateFindBin();

protected:
    void SetUp() override;

    UniquePtr<Bvh>   bvh_;
    mlm::Ray         hitRay_;
    mlm::PackedVec3  worldMax_;

private:
    static bool isValidRecursive(const Bvh::Node& node);
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
    mlm::Bound     worldBound;
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

    return isValidRecursive(bvh.nodes_[0]);
}

// NOLINTNEXTLINE(misc-no-recursion)
bool BvhTest::isValidRecursive(const Bvh::Node& node)
{
    if (!node.left)
    {
        return true;
    }

    const Bvh::Node& left = *node.left;
    const Bvh::Node& right = *node.right;

    bool isChildValid = left.triCount + right.triCount == node.triCount &&
                        left.triStart == node.triStart &&
                        right.triStart == node.triStart + left.triCount &&
                        node.localBound.doesInclude(left.localBound) &&
                        node.localBound.doesInclude(right.localBound);

    return isChildValid && isValidRecursive(left) && isValidRecursive(right);
}

void BvhTest::validateRecursiveCases()
{
    mlm::Bound leafBound(mlm::PackedVec3(0.f, 0.f, 0.f), mlm::PackedVec3(1.f, 1.f, 1.f));
    mlm::Bound rightBound(mlm::PackedVec3(2.f, 2.f, 2.f), mlm::PackedVec3(3.f, 3.f, 3.f));
    mlm::Bound parentBound(mlm::PackedVec3(0.f, 0.f, 0.f), mlm::PackedVec3(3.f, 3.f, 3.f));
    mlm::Bound outsideBound(mlm::PackedVec3(5.f, 5.f, 5.f), mlm::PackedVec3(6.f, 6.f, 6.f));

    auto makeLeaf = [](u32 triStart, u32 triCount, mlm::Bound bound)
    {
        return Bvh::Node{ .left = nullptr, .right = nullptr, .localBound = bound, .triStart = triStart, .triCount = triCount };
    };

    // Leaf is always valid.
    EXPECT_TRUE(isValidRecursive(makeLeaf(0, 1, leafBound)));

    // Internal node with consistent children.
    Bvh::Node left = makeLeaf(0, 2, leafBound);
    Bvh::Node right = makeLeaf(2, 3, rightBound);
    Bvh::Node parent = { .left = &left, .right = &right, .localBound = parentBound, .triStart = 0, .triCount = 5 };
    EXPECT_TRUE(isValidRecursive(parent));

    // right.triStart must equal node.triStart + left.triCount.
    Bvh::Node rightBadStart = makeLeaf(3, 3, rightBound);
    Bvh::Node parentBadStart = { .left = &left, .right = &rightBadStart, .localBound = parentBound, .triStart = 0, .triCount = 5 };
    EXPECT_FALSE(isValidRecursive(parentBadStart));

    // Child triCounts must sum to the parent's.
    Bvh::Node leftBadCount = makeLeaf(0, 3, leafBound);
    Bvh::Node parentBadCount = { .left = &leftBadCount, .right = &right, .localBound = parentBound, .triStart = 0, .triCount = 5 };
    EXPECT_FALSE(isValidRecursive(parentBadCount));

    // Child bound must be contained by the parent's.
    Bvh::Node leftOutside = makeLeaf(0, 2, outsideBound);
    Bvh::Node parentBadBound = { .left = &leftOutside, .right = &right, .localBound = parentBound, .triStart = 0, .triCount = 5 };
    EXPECT_FALSE(isValidRecursive(parentBadBound));
}

void BvhTest::validateSplitAxis()
{
    EXPECT_EQ(Bvh::chooseSplitAxis(mlm::Bound(mlm::PackedVec3(0.f, 0.f, 0.f), mlm::PackedVec3(3.f, 1.f, 1.f))),
              Bvh::Axis::eX);
    EXPECT_EQ(Bvh::chooseSplitAxis(mlm::Bound(mlm::PackedVec3(0.f, 0.f, 0.f), mlm::PackedVec3(1.f, 3.f, 1.f))),
              Bvh::Axis::eY);
    EXPECT_EQ(Bvh::chooseSplitAxis(mlm::Bound(mlm::PackedVec3(0.f, 0.f, 0.f), mlm::PackedVec3(1.f, 1.f, 3.f))),
              Bvh::Axis::eZ);
}

void BvhTest::validateFindBin()
{
    mlm::Bound bound(mlm::PackedVec3(0.f, 0.f, 0.f), mlm::PackedVec3(12.f, 0.f, 0.f));

    Bvh::Tri atMin = { .indexStart = 0, .centeroid = mlm::PackedVec3(0.f, 0.f, 0.f), .bound = {} };
    Bvh::Tri atMid = { .indexStart = 0, .centeroid = mlm::PackedVec3(6.f, 0.f, 0.f), .bound = {} };
    Bvh::Tri atMax = { .indexStart = 0, .centeroid = mlm::PackedVec3(12.f, 0.f, 0.f), .bound = {} };

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
    mlm::Ray r{ .o = worldMax_ + mlm::PackedVec3(0.f, 0.f, 10.f), .d = mlm::PackedVec3(0.f, 0.f, 1.f) };
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
