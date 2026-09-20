#include <test/ISimpleTest.h>
#include <core/mlm/IBasis.h>

namespace mk::mlm
{

static constexpr float kEps = 1e-5f;

#define EXPECT_F(a, b) EXPECT_NEAR((a), (b), kEps)

namespace
{
void expectOrthonormal(const Basis& b)
{
    EXPECT_TRUE(isNormalized(b.x(), kEps));
    EXPECT_TRUE(isNormalized(b.y(), kEps));
    EXPECT_TRUE(isNormalized(b.z(), kEps));

    EXPECT_NEAR(dot(b.x(), b.y()), 0.f, kEps);
    EXPECT_NEAR(dot(b.y(), b.z()), 0.f, kEps);
    EXPECT_NEAR(dot(b.z(), b.x()), 0.f, kEps);
}

void expectLeftHanded(const Basis& b)
{
    // Left-handed y-up convention: cross(x, y) == -z.
    PackedVec3 c = cross(b.x(), b.y());
    EXPECT_F(c.x(), -b.z().x());
    EXPECT_F(c.y(), -b.z().y());
    EXPECT_F(c.z(), -b.z().z());
}

const PackedVec3 kAxisNormals[] = {
    PackedVec3(1.f, 0.f, 0.f),  PackedVec3(-1.f, 0.f, 0.f),  PackedVec3(0.f, 1.f, 0.f),
    PackedVec3(0.f, -1.f, 0.f), PackedVec3(0.f, 0.f, 1.f),   PackedVec3(0.f, 0.f, -1.f),
    normalize(PackedVec3(1.f, 2.f, 3.f)),
};
} // namespace

// ------------------------------ Pair factories ------------------------------

TEST(BasisPair, Xy)
{
    PackedVec3 x(1.f, 0.f, 0.f);
    PackedVec3 y(0.f, 1.f, 0.f);

    Basis b = Basis::xy(x, y);

    EXPECT_EQ(b.x(), x);
    EXPECT_EQ(b.y(), y);
    EXPECT_EQ(b.z(), cross(y, x));
    expectOrthonormal(b);
    expectLeftHanded(b);
}

TEST(BasisPair, Xz)
{
    PackedVec3 x(1.f, 0.f, 0.f);
    PackedVec3 z(0.f, 0.f, 1.f);

    Basis b = Basis::xz(x, z);

    EXPECT_EQ(b.x(), x);
    EXPECT_EQ(b.y(), cross(x, z));
    EXPECT_EQ(b.z(), z);
    expectOrthonormal(b);
    expectLeftHanded(b);
}

TEST(BasisPair, Yz)
{
    PackedVec3 y(0.f, 1.f, 0.f);
    PackedVec3 z(0.f, 0.f, 1.f);

    Basis b = Basis::yz(y, z);

    EXPECT_EQ(b.x(), cross(z, y));
    EXPECT_EQ(b.y(), y);
    EXPECT_EQ(b.z(), z);
    expectOrthonormal(b);
    expectLeftHanded(b);
}

// ----------------------------- Single-axis factories ------------------------

TEST(BasisSingle, XAxis)
{
    for (PackedVec3 n : kAxisNormals)
    {
        Basis b = Basis::x(n);
        EXPECT_EQ(b.x(), n);
        expectOrthonormal(b);
        expectLeftHanded(b);
    }
}

TEST(BasisSingle, YAxis)
{
    for (PackedVec3 n : kAxisNormals)
    {
        Basis b = Basis::y(n);
        EXPECT_EQ(b.y(), n);
        expectOrthonormal(b);
        expectLeftHanded(b);
    }
}

TEST(BasisSingle, ZAxis)
{
    for (PackedVec3 n : kAxisNormals)
    {
        Basis b = Basis::z(n);
        EXPECT_EQ(b.z(), n);
        expectOrthonormal(b);
        expectLeftHanded(b);
    }
}

} // namespace mk::mlm
MK_SIMPLE_MAIN()

