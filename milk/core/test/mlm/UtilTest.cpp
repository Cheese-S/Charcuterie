#include <test/ISimpleTest.h>
#include <core/mlm/IUtil.h>

namespace mk::mlm
{

static constexpr float kEps = 1e-5f;

#define EXPECT_F(a, b) EXPECT_NEAR((a), (b), kEps)

namespace
{
// Builds a unit cube from (-1,-1,-1) to (1,1,1).
Bound makeUnitBox()
{
    return Bound(PackedVec3{ -1.f, -1.f, -1.f }, PackedVec3{ 1.f, 1.f, 1.f });
}

// Precomputes invD and dirIsNeg the way a caller of rayBoundIntersection is
// expected to, per the header's "dirIsNeg[x] can only be 0 or 1" contract.
struct RayPrep
{
    PackedVec3     invD;
    u8             negBuf[3];
    VectorView<u8> dirIsNeg;

    explicit RayPrep(const Ray& r):
        invD{ 1.f / r.d.x(), 1.f / r.d.y(), 1.f / r.d.z() },
        negBuf{ static_cast<u8>(r.d.x() < 0.f ? 1 : 0),
                static_cast<u8>(r.d.y() < 0.f ? 1 : 0),
                static_cast<u8>(r.d.z() < 0.f ? 1 : 0) },
        dirIsNeg(negBuf, 3)
    {
    }
};

bool intersects(const Ray& r, const Bound& b, f32 rtMax = FLT_MAX)
{
    RayPrep prep(r);
    return rayBoundIntersection(r, b, rtMax, prep.invD, prep.dirIsNeg);
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

// ------------------------------- Basic hits ---------------------------------

TEST(RayBoundIntersection, HitAlongPositiveXAxis)
{
    Bound b = makeUnitBox();
    Ray   r{ .o = PackedVec3{ -5.f, 0.f, 0.f }, .d = PackedVec3{ 1.f, 0.f, 0.f } };
    EXPECT_TRUE(intersects(r, b));
}

TEST(RayBoundIntersection, HitAlongNegativeXAxis)
{
    // Exercises the dirIsNeg[0] == 1 branch.
    Bound b = makeUnitBox();
    Ray   r{ .o = PackedVec3{ 5.f, 0.f, 0.f }, .d = PackedVec3{ -1.f, 0.f, 0.f } };
    EXPECT_TRUE(intersects(r, b));
}

TEST(RayBoundIntersection, HitAlongYAxis)
{
    Bound b = makeUnitBox();
    Ray   r{ .o = PackedVec3{ 0.f, -5.f, 0.f }, .d = PackedVec3{ 0.f, 1.f, 0.f } };
    EXPECT_TRUE(intersects(r, b));
}

TEST(RayBoundIntersection, HitAlongZAxis)
{
    Bound b = makeUnitBox();
    Ray   r{ .o = PackedVec3{ 0.f, 0.f, -5.f }, .d = PackedVec3{ 0.f, 0.f, 1.f } };
    EXPECT_TRUE(intersects(r, b));
}

TEST(RayBoundIntersection, HitDiagonalRay)
{
    Bound b = makeUnitBox();
    Ray   r{ .o = PackedVec3{ -5.f, -5.f, -5.f }, .d = PackedVec3{ 1.f, 1.f, 1.f } };
    EXPECT_TRUE(intersects(r, b));
}

TEST(RayBoundIntersection, OriginInsideBoxAlwaysHits)
{
    Bound b = makeUnitBox();
    Ray   r{ .o = PackedVec3{ 0.f, 0.f, 0.f }, .d = PackedVec3{ 1.f, 0.f, 0.f } };
    EXPECT_TRUE(intersects(r, b));
}

// ------------------------------- Basic misses --------------------------------

TEST(RayBoundIntersection, MissParallelOffsetRay)
{
    // Ray travels along +X but is offset far outside the box on Y.
    Bound b = makeUnitBox();
    Ray   r{ .o = PackedVec3{ -5.f, 5.f, 0.f }, .d = PackedVec3{ 1.f, 0.f, 0.f } };
    EXPECT_FALSE(intersects(r, b));
}

TEST(RayBoundIntersection, MissRayPointingAwayFromBox)
{
    // Box is behind the ray's origin given its direction; a correct slab
    // test should report no intersection (t < 0 rejected).
    Bound b = makeUnitBox();
    Ray   r{ .o = PackedVec3{ -5.f, 0.f, 0.f }, .d = PackedVec3{ -1.f, 0.f, 0.f } };
    EXPECT_FALSE(intersects(r, b));
}

TEST(RayBoundIntersection, MissDiagonalRayEntirelyOffset)
{
    Bound b = makeUnitBox();
    Ray   r{ .o = PackedVec3{ -5.f, -5.f, 10.f }, .d = PackedVec3{ 1.f, 1.f, 1.f } };
    EXPECT_FALSE(intersects(r, b));
}

TEST(RayBoundIntersection, MissDiagonalRayAboveBox)
{
    // Regression test: the Y-slab's tyMax must be folded into tMax. A ray that
    // leaves the box's Y extent before entering its Z extent is a miss, but the
    // old `tMax = std::min(tMax, tMax)` bug reported a hit.
    Bound b = makeUnitBox();
    Ray   r{ .o = PackedVec3{ 0.f, 1.5f, -1.5f }, .d = PackedVec3{ 1.f, 1.f, 1.f } };
    EXPECT_FALSE(intersects(r, b));
}

TEST(RayBoundIntersection, MissDiagonalRayBelowBox)
{
    // Mirrored case (negative Y direction) for the same regression.
    Bound b = makeUnitBox();
    Ray   r{ .o = PackedVec3{ 0.f, -1.5f, -1.5f }, .d = PackedVec3{ 1.f, -1.f, 1.f } };
    EXPECT_FALSE(intersects(r, b));
}

// ------------------------------- Edge / tangent cases -------------------------

TEST(RayBoundIntersection, GrazingEdgeOfBox)
{
    // Ray travels exactly along the box's max-Y face (y = 1). Whether this
    // counts as a hit is an implementation choice (closed vs open interval);
    // document actual behavior here once implementation is available.
    Bound b = makeUnitBox();
    Ray   r{ .o = PackedVec3{ -5.f, 1.f, 0.f }, .d = PackedVec3{ 1.f, 0.f, 0.f } };
    // Expect a hit under a closed-interval (<=) convention.
    EXPECT_TRUE(intersects(r, b));
}

TEST(RayBoundIntersection, ParallelToSlabOriginOutsideRange)
{
    // Direction has zero X component; invD.x() becomes +/-inf. Ray's X origin
    // is outside [-1, 1], so it must miss regardless of Y/Z motion.
    Bound b = makeUnitBox();
    Ray   r{ .o = PackedVec3{ 5.f, -5.f, 0.f }, .d = PackedVec3{ 0.f, 1.f, 0.f } };
    EXPECT_FALSE(intersects(r, b));
}

TEST(RayBoundIntersection, ParallelToSlabOriginInsideRange)
{
    // Direction has zero X component, but ray's X origin (0) is within
    // [-1, 1], so intersection depends only on the other axes.
    Bound b = makeUnitBox();
    Ray   r{ .o = PackedVec3{ 0.f, -5.f, 0.f }, .d = PackedVec3{ 0.f, 1.f, 0.f } };
    EXPECT_TRUE(intersects(r, b));
}

// ------------------------------- rtMax clipping --------------------------------

TEST(RayBoundIntersection, RtMaxSmallerThanHitDistanceMisses)
{
    Bound b = makeUnitBox();
    Ray   r{ .o = PackedVec3{ -5.f, 0.f, 0.f }, .d = PackedVec3{ 1.f, 0.f, 0.f } };
    // True hit distance to box is 4 (from x=-5 to x=-1). Clip well before that.
    EXPECT_FALSE(intersects(r, b, /*rtMax=*/1.0f));
}

TEST(RayBoundIntersection, RtMaxLargerThanHitDistanceHits)
{
    Bound b = makeUnitBox();
    Ray   r{ .o = PackedVec3{ -5.f, 0.f, 0.f }, .d = PackedVec3{ 1.f, 0.f, 0.f } };
    EXPECT_TRUE(intersects(r, b, /*rtMax=*/10.0f));
}

// ------------------------------- Degenerate bound --------------------------------

TEST(RayBoundIntersection, DefaultBoundNeverIntersects)
{
    Bound b; // degenerate/empty
    Ray   r{ .o = PackedVec3{ 0.f, 0.f, 0.f }, .d = PackedVec3{ 1.f, 0.f, 0.f } };
    EXPECT_FALSE(intersects(r, b));
}

// ------------------------------- Non-cubic / off-origin bound --------------------

TEST(RayBoundIntersection, OffCenterAsymmetricBoxHit)
{
    Bound b(PackedVec3{ 2.f, -1.f, 4.f }, PackedVec3{ 6.f, 3.f, 8.f });
    Ray   r{ .o = PackedVec3{ 0.f, 1.f, 6.f }, .d = PackedVec3{ 1.f, 0.f, 0.f } };
    EXPECT_TRUE(intersects(r, b));
}

TEST(RayBoundIntersection, OffCenterAsymmetricBoxMiss)
{
    Bound b(PackedVec3{ 2.f, -1.f, 4.f }, PackedVec3{ 6.f, 3.f, 8.f });
    Ray   r{ .o = PackedVec3{ 0.f, 10.f, 6.f }, .d = PackedVec3{ 1.f, 0.f, 0.f } };
    EXPECT_FALSE(intersects(r, b));
}

// ------------------------------ TRANSFORM RAY ------------------------------ //

TEST(TransformRay, Identity)
{
    Ray  r{ .o = PackedVec3{ 1.f, 2.f, 3.f }, .d = PackedVec3{ 1.f, 0.f, 0.f } };
    mat4 I;
    Ray  result = transformRay(I, r);
    EXPECT_EQ(result.o, r.o);
    EXPECT_EQ(result.d, r.d);
}

TEST(TransformRay, Translate)
{
    Ray  r{ .o = PackedVec3{ 1.f, 2.f, 3.f }, .d = PackedVec3{ 1.f, 0.f, 0.f } };
    mat4 t = mat4::translate(5.f, -3.f, 2.f);
    Ray  result = transformRay(t, r);
    EXPECT_EQ(result.o, PackedVec3(6.f, -1.f, 5.f));
    EXPECT_EQ(result.d, PackedVec3(1.f, 0.f, 0.f));
}

TEST(TransformRay, Rotate)
{
    Ray   r{ .o = PackedVec3{ 1.f, 2.f, 3.f }, .d = PackedVec3{ 1.f, 0.f, 0.f } };
    float halfSqrt2 = 0.70710678f;
    quat  q(0.f, 0.f, halfSqrt2, halfSqrt2);
    mat4  R = mat4::rotate(q);
    Ray   result = transformRay(R, r);
    EXPECT_F(result.o.x(), -2.f);
    EXPECT_F(result.o.y(), 1.f);
    EXPECT_F(result.o.z(), 3.f);
    EXPECT_F(result.d.x(), 0.f);
    EXPECT_F(result.d.y(), 1.f);
    EXPECT_F(result.d.z(), 0.f);
}

TEST(TransformRay, ScaleUniform)
{
    Ray  r{ .o = PackedVec3{ 1.f, 2.f, 3.f }, .d = PackedVec3{ 1.f, 0.f, 0.f } };
    mat4 s = mat4::scale(2.f);
    Ray  result = transformRay(s, r);
    EXPECT_EQ(result.o, PackedVec3(2.f, 4.f, 6.f));
    EXPECT_EQ(result.d, PackedVec3(1.f, 0.f, 0.f));
}

TEST(TransformRay, Chain)
{
    Ray   r{ .o = PackedVec3{ 1.f, 2.f, 3.f }, .d = PackedVec3{ 1.f, 0.f, 0.f } };
    mat4  t = mat4::translate(5.f, 0.f, 0.f);
    float halfSqrt2 = 0.70710678f;
    quat  q(0.f, 0.f, halfSqrt2, halfSqrt2);
    mat4  R = mat4::rotate(q);
    mat4  M = R * t;
    Ray   result = transformRay(M, r);
    EXPECT_F(result.o.x(), -2.f);
    EXPECT_F(result.o.y(), 6.f);
    EXPECT_F(result.o.z(), 3.f);
    EXPECT_F(result.d.x(), 0.f);
    EXPECT_F(result.d.y(), 1.f);
    EXPECT_F(result.d.z(), 0.f);
}

} // namespace mk::mlm
MK_SIMPLE_MAIN()
