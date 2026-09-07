#include <test/ISimpleTest.h>
#include <core/mlm/IBound.h>

namespace mk::mlm
{

namespace
{
// Builds a unit cube from (-1,-1,-1) to (1,1,1).
Bound makeUnitBox()
{
    return Bound(PackedVec3{ -1.f, -1.f, -1.f }, PackedVec3{ 1.f, 1.f, 1.f });
}
} // namespace

// ------------------------------- Bound sanity -------------------------------

TEST(Bound, MinMaxAccessorsMatchConstructor)
{
    Bound b = makeUnitBox();
    EXPECT_FLOAT_EQ(b.min().x(), -1.f);
    EXPECT_FLOAT_EQ(b.max().x(), 1.f);
}

// Regression test for the operator[] bug described at the top of this file.
// This SHOULD pass once operator[] correctly indexes into extents_.
TEST(Bound, OperatorBracketMatchesMinMax)
{
    Bound b = makeUnitBox();
    EXPECT_FLOAT_EQ(b[0].x(), b.min().x());
    EXPECT_FLOAT_EQ(b[1].x(), b.max().x()); // fails today: operator[] always returns extents_[0]
    EXPECT_NE(b[0].x(), b[1].x());          // fails today for the same reason
}

TEST(Bound, DefaultConstructedBoundIsDegenerate)
{
    // Default ctor sets min = {FLT_MAX,...}, max = {FLT_MIN,...}.
    // Note FLT_MIN is the smallest positive normal float (~1.18e-38), not the
    // most negative float, so this only produces an "inverted" (min > max)
    // box, and only meaningfully so for the empty()/include() logic -- any
    // ray/bound test against it should report no intersection.
    Bound b;
    EXPECT_TRUE(b.empty());
}

TEST(Bound, IncludePt)
{
    Bound b;
    b.toInclude(PackedVec3(0, 0, 0));
    EXPECT_EQ(b.min(), PackedVec3(0.f, 0.f, 0.f));
    EXPECT_EQ(b.max(), PackedVec3(0.f, 0.f, 0.f));
    EXPECT_FALSE(b.empty());

    b.toInclude(PackedVec3(2.f, -3.f, 5.f));
    EXPECT_EQ(b.min(), PackedVec3(0.f, -3.f, 0.f));
    EXPECT_EQ(b.max(), PackedVec3(2.f, 0.f, 5.f));
}

TEST(Bound, IncludeBound)
{
    Bound a(PackedVec3(-1.f, -1.f, -1.f), PackedVec3(1.f, 1.f, 1.f));
    Bound b(PackedVec3(2.f, 0.f, -3.f), PackedVec3(4.f, 5.f, 2.f));
    a.toInclude(b);
    EXPECT_EQ(a.min(), PackedVec3(-1.f, -1.f, -3.f));
    EXPECT_EQ(a.max(), PackedVec3(4.f, 5.f, 2.f));
}

TEST(Bound, IncludeBoundEmpty)
{
    Bound a(PackedVec3(-1.f, -1.f, -1.f), PackedVec3(1.f, 1.f, 1.f));
    Bound b;
    a.toInclude(b);
    EXPECT_EQ(a.min(), PackedVec3(-1.f, -1.f, -1.f));
    EXPECT_EQ(a.max(), PackedVec3(1.f, 1.f, 1.f));
}

TEST(Bound, SurfaceAreaUnitBox)
{
    Bound b = makeUnitBox();
    EXPECT_FLOAT_EQ(b.surfaceArea(), 24.f);
}

TEST(Bound, SurfaceAreaZeroSizeBox)
{
    Bound b(PackedVec3(0.f, 0.f, 0.f), PackedVec3(0.f, 0.f, 0.f));
    EXPECT_FLOAT_EQ(b.surfaceArea(), 0.f);
}

TEST(Bound, SurfaceAreaAsymmetricBox)
{
    Bound b(PackedVec3(0.f, 0.f, 0.f), PackedVec3(2.f, 3.f, 4.f));
    EXPECT_FLOAT_EQ(b.surfaceArea(), 52.f);
}

TEST(Bound, EmptyAfterInclude)
{
    Bound b;
    EXPECT_TRUE(b.empty());
    b.toInclude(PackedVec3(0, 0, 0));
    EXPECT_FALSE(b.empty());
}

// --------------------------- doesInclude (point) ---------------------------

TEST(Bound, DoesIncludePointInside)
{
    Bound b = makeUnitBox();
    EXPECT_TRUE(b.doesInclude(PackedVec3(0.f, 0.f, 0.f)));
}

TEST(Bound, DoesIncludePointOnMin)
{
    Bound b = makeUnitBox();
    EXPECT_TRUE(b.doesInclude(PackedVec3(-1.f, -1.f, -1.f)));
}

TEST(Bound, DoesIncludePointOnMax)
{
    Bound b = makeUnitBox();
    EXPECT_TRUE(b.doesInclude(PackedVec3(1.f, 1.f, 1.f)));
}

TEST(Bound, DoesIncludePointOutsideX)
{
    Bound b = makeUnitBox();
    EXPECT_FALSE(b.doesInclude(PackedVec3(2.f, 0.f, 0.f)));
    EXPECT_FALSE(b.doesInclude(PackedVec3(-2.f, 0.f, 0.f)));
}

TEST(Bound, DoesIncludePointOutsideY)
{
    Bound b = makeUnitBox();
    EXPECT_FALSE(b.doesInclude(PackedVec3(0.f, 2.f, 0.f)));
    EXPECT_FALSE(b.doesInclude(PackedVec3(0.f, -2.f, 0.f)));
}

TEST(Bound, DoesIncludePointOutsideZ)
{
    Bound b = makeUnitBox();
    EXPECT_FALSE(b.doesInclude(PackedVec3(0.f, 0.f, 2.f)));
    EXPECT_FALSE(b.doesInclude(PackedVec3(0.f, 0.f, -2.f)));
}

TEST(Bound, DoesIncludePointEmptyBound)
{
    Bound b;
    EXPECT_FALSE(b.doesInclude(PackedVec3(0.f, 0.f, 0.f)));
}

// --------------------------- doesInclude (bound) ---------------------------

TEST(Bound, DoesIncludeBoundStrictlyInside)
{
    Bound b = makeUnitBox();
    Bound inner(PackedVec3(-0.5f, -0.5f, -0.5f), PackedVec3(0.5f, 0.5f, 0.5f));
    EXPECT_TRUE(b.doesInclude(inner));
}

TEST(Bound, DoesIncludeBoundEqual)
{
    Bound b = makeUnitBox();
    EXPECT_TRUE(b.doesInclude(b));
}

TEST(Bound, DoesIncludeBoundPartialOverlap)
{
    Bound b = makeUnitBox();
    Bound other(PackedVec3(0.5f, 0.5f, 0.5f), PackedVec3(2.f, 2.f, 2.f));
    EXPECT_FALSE(b.doesInclude(other));
}

TEST(Bound, DoesIncludeBoundContainsThis)
{
    Bound b = makeUnitBox();
    Bound larger(PackedVec3(-2.f, -2.f, -2.f), PackedVec3(2.f, 2.f, 2.f));
    EXPECT_FALSE(b.doesInclude(larger));
}

TEST(Bound, DoesIncludeBoundDisjoint)
{
    Bound b = makeUnitBox();
    Bound other(PackedVec3(3.f, 3.f, 3.f), PackedVec3(4.f, 4.f, 4.f));
    EXPECT_FALSE(b.doesInclude(other));
}

TEST(Bound, DoesIncludeBoundEmpty)
{
    // An empty (degenerate) bound is trivially a subset of every bound.
    Bound b = makeUnitBox();
    Bound empty;
    EXPECT_TRUE(b.doesInclude(empty));
}

TEST(Bound, MinMaxMutableAccessors)
{
    Bound b;
    b.min() = PackedVec3(-2.f, -3.f, -4.f);
    b.max() = PackedVec3(2.f, 3.f, 4.f);
    EXPECT_EQ(b.min(), PackedVec3(-2.f, -3.f, -4.f));
    EXPECT_EQ(b.max(), PackedVec3(2.f, 3.f, 4.f));

    b[0] = PackedVec3(-5.f, -6.f, -7.f);
    b[1] = PackedVec3(5.f, 6.f, 7.f);
    EXPECT_EQ(b[0], PackedVec3(-5.f, -6.f, -7.f));
    EXPECT_EQ(b[1], PackedVec3(5.f, 6.f, 7.f));
}

} // namespace mk::mlm
MK_SIMPLE_MAIN()
