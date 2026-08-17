#include <test/ISimpleTest.h>
#include <core/mlm/IMlm.h>

namespace mk::mlm
{

static constexpr float kEps = 1e-5f;

#define EXPECT_F(a, b) EXPECT_NEAR((a), (b), kEps)

//
// ============================================================
// Vec Tests
// ============================================================
//

TEST(Vec3Test, Accessors)
{
    vec3 v(1.0f, 2.0f, 3.0f);

    EXPECT_F(v.x(), 1.0f);
    EXPECT_F(v.y(), 2.0f);
    EXPECT_F(v.z(), 3.0f);

    v.x() = 10.0f;
    v.y() = 20.0f;
    v.z() = 30.0f;

    EXPECT_F(v.x(), 10.0f);
    EXPECT_F(v.y(), 20.0f);
    EXPECT_F(v.z(), 30.0f);
}

TEST(Vec4Test, Accessors)
{
    vec4 v(1.0f, 2.0f, 3.0f, 4.0f);

    EXPECT_F(v.x(), 1.0f);
    EXPECT_F(v.y(), 2.0f);
    EXPECT_F(v.z(), 3.0f);
    EXPECT_F(v.w(), 4.0f);

    v.x() = 11.0f;
    v.y() = 22.0f;
    v.z() = 33.0f;
    v.w() = 44.0f;

    EXPECT_F(v.x(), 11.0f);
    EXPECT_F(v.y(), 22.0f);
    EXPECT_F(v.z(), 33.0f);
    EXPECT_F(v.w(), 44.0f);
}

TEST(VecTest, Indexing)
{
    vec3 v(5.0f, 6.0f, 7.0f);

    EXPECT_F(v[0], 5.0f);
    EXPECT_F(v[1], 6.0f);
    EXPECT_F(v[2], 7.0f);

    v[0] = 50.0f;
    v[1] = 60.0f;
    v[2] = 70.0f;

    EXPECT_F(v[0], 50.0f);
    EXPECT_F(v[1], 60.0f);
    EXPECT_F(v[2], 70.0f);
}

TEST(VecTest, multiplyByConst)
{
    vec4 v(1, 2, 3, 4);

    v = 3 * v;

    EXPECT_F(v[0], 3);
    EXPECT_F(v[1], 6);
    EXPECT_F(v[2], 9);
    EXPECT_F(v[3], 12);
}

TEST(VecTest, equality)
{
    vec2 a2(1, 1);
    vec2 b2(1, 1);
    EXPECT_EQ(a2, b2);

    vec3 a3(1, 1, 1);
    vec3 b3(1, 1, 1);
    EXPECT_EQ(a3, b3);

    vec4 a4(1, 1, 1, 1);
    vec4 b4(1, 1, 1, 1);
    EXPECT_EQ(a4, b4);
}

//
// ============================================================
// Mat4 Tests (FULL COVERAGE)
// ============================================================
//

TEST(Mat4Test, Identity)
{
    mat4 m;

    // Column 0
    EXPECT_F(m[0][0], 1.0f);
    EXPECT_F(m[0][1], 0.0f);
    EXPECT_F(m[0][2], 0.0f);
    EXPECT_F(m[0][3], 0.0f);

    // Column 1
    EXPECT_F(m[1][0], 0.0f);
    EXPECT_F(m[1][1], 1.0f);
    EXPECT_F(m[1][2], 0.0f);
    EXPECT_F(m[1][3], 0.0f);

    // Column 2
    EXPECT_F(m[2][0], 0.0f);
    EXPECT_F(m[2][1], 0.0f);
    EXPECT_F(m[2][2], 1.0f);
    EXPECT_F(m[2][3], 0.0f);

    // Column 3
    EXPECT_F(m[3][0], 0.0f);
    EXPECT_F(m[3][1], 0.0f);
    EXPECT_F(m[3][2], 0.0f);
    EXPECT_F(m[3][3], 1.0f);
}

TEST(Mat4Test, ConstructionFromElements)
{
    // clang-format off
    mat4 m( 1, 2, 3, 4, 
            5, 6, 7, 8, 
            9, 10, 11, 12, 
            13, 14, 15, 16);
    // clang-format on

    // Column-major verification (CRITICAL)
    EXPECT_F(m[0][0], 1);
    EXPECT_F(m[0][1], 5);
    EXPECT_F(m[0][2], 9);
    EXPECT_F(m[0][3], 13);

    EXPECT_F(m[1][0], 2);
    EXPECT_F(m[1][1], 6);
    EXPECT_F(m[1][2], 10);
    EXPECT_F(m[1][3], 14);

    EXPECT_F(m[2][0], 3);
    EXPECT_F(m[2][1], 7);
    EXPECT_F(m[2][2], 11);
    EXPECT_F(m[2][3], 15);

    EXPECT_F(m[3][0], 4);
    EXPECT_F(m[3][1], 8);
    EXPECT_F(m[3][2], 12);
    EXPECT_F(m[3][3], 16);
}

TEST(Mat4Test, MatrixVectorMultiplyIdentity)
{
    mat4 m;
    vec4 v(1, 2, 3, 4);

    vec4 r = m * v;

    EXPECT_F(r.x(), 1);
    EXPECT_F(r.y(), 2);
    EXPECT_F(r.z(), 3);
    EXPECT_F(r.w(), 4);
}

TEST(Mat4Test, MatrixMatrixMultiplyIdentityPreservesAllElements)
{
    mat4 a;

    mat4 b(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16);

    mat4 c = a * b;

    // FULL 4x4 verification
    for (int col = 0; col < 4; ++col)
    {
        for (int row = 0; row < 4; ++row)
        {
            EXPECT_F(c[col][row], b[col][row]);
        }
    }
}

TEST(Mat4Test, equality)
{
    mat4 a;
    mat4 b;
    EXPECT_TRUE(a == b);
}

TEST(Mat4Test, ScaleUniform)
{
    // mat4::scale(s) should scale x, y, z by s and leave w unchanged.
    mat4 s = mat4::scale(3.0f);
    vec4 p(1.0f, 2.0f, 3.0f, 1.0f); // point
    vec4 r = s * p;
    EXPECT_F(r.x(), 3.0f);
    EXPECT_F(r.y(), 6.0f);
    EXPECT_F(r.z(), 9.0f);
    EXPECT_F(r.w(), 1.0f);

    vec4 d(1.0f, 0.0f, 0.0f, 0.0f); // direction
    r = s * d;
    EXPECT_F(r.x(), 3.0f);
    EXPECT_F(r.y(), 0.0f);
    EXPECT_F(r.z(), 0.0f);
    EXPECT_F(r.w(), 0.0f);
}

TEST(Mat4Test, ScaleNonUniform)
{
    mat4 s = mat4::scale(2.0f, 3.0f, 4.0f);
    vec4 p(1.0f, 1.0f, 1.0f, 1.0f);
    vec4 r = s * p;
    EXPECT_F(r.x(), 2.0f);
    EXPECT_F(r.y(), 3.0f);
    EXPECT_F(r.z(), 4.0f);
    EXPECT_F(r.w(), 1.0f);
}

TEST(Mat4Test, TranslatePoint)
{
    mat4 t = mat4::translate(5.0f, -3.0f, 2.0f);
    vec4 p(1.0f, 2.0f, 3.0f, 1.0f);
    vec4 r = t * p;
    EXPECT_F(r.x(), 6.0f);
    EXPECT_F(r.y(), -1.0f);
    EXPECT_F(r.z(), 5.0f);
    EXPECT_F(r.w(), 1.0f);
}

TEST(Mat4Test, TranslateDoesNotAffectDirection)
{
    mat4 t = mat4::translate(100.0f, 200.0f, 300.0f);
    vec4 d(1.0f, 0.0f, 0.0f, 0.0f);
    vec4 r = t * d;
    EXPECT_F(r.x(), 1.0f);
    EXPECT_F(r.y(), 0.0f);
    EXPECT_F(r.z(), 0.0f);
    EXPECT_F(r.w(), 0.0f);
}

TEST(Mat4Test, InverseIdentity)
{
    mat4 I;
    mat4 invI = mat4::inverse(I);
    EXPECT_TRUE(invI == I);
}

TEST(Mat4Test, InverseScale)
{
    mat4 s = mat4::scale(2.0f, 4.0f, 8.0f);
    mat4 invS = mat4::inverse(s);

    // Verify that invS * s == identity
    mat4 prod = invS * s;
    mat4 I;
    for (int col = 0; col < 4; ++col)
    {
        for (int row = 0; row < 4; ++row)
        {
            EXPECT_F(prod[col][row], I[col][row]);
        }
    }
}

TEST(Mat4Test, InverseTranslate)
{
    mat4 t = mat4::translate(10.0f, 20.0f, 30.0f);
    mat4 invT = mat4::inverse(t);

    // Applying invT should bring a translated point back to origin
    vec4 p(1.0f, 1.0f, 1.0f, 1.0f);
    vec4 translated = t * p;
    vec4 back = invT * translated;
    EXPECT_F(back.x(), 1.0f);
    EXPECT_F(back.y(), 1.0f);
    EXPECT_F(back.z(), 1.0f);
    EXPECT_F(back.w(), 1.0f);

    // Also verify full product is identity
    mat4 prod = invT * t;
    mat4 I;
    for (int col = 0; col < 4; ++col)
    {
        for (int row = 0; row < 4; ++row)
        {
            EXPECT_F(prod[col][row], I[col][row]);
        }
    }
}

TEST(Mat4Test, RotateAroundZ90)
{
    // 90° rotation around Z axis: quaternion (x=0, y=0, z=sin45°, w=cos45°)
    // Library quaternion constructor is quat(x, y, z, w)
    float halfSqrt2 = 0.70710678f;
    quat  q(0.0f, 0.0f, halfSqrt2, halfSqrt2);
    mat4  R = mat4::rotate(q);

    // X axis should map to Y axis
    vec4 xAxis(1.0f, 0.0f, 0.0f, 0.0f);
    vec4 r = R * xAxis;
    EXPECT_F(r.x(), 0.0f);
    EXPECT_F(r.y(), 1.0f);
    EXPECT_F(r.z(), 0.0f);
    EXPECT_F(r.w(), 0.0f);

    // Y axis should map to -X axis
    vec4 yAxis(0.0f, 1.0f, 0.0f, 0.0f);
    r = R * yAxis;
    EXPECT_F(r.x(), -1.0f);
    EXPECT_F(r.y(), 0.0f);
    EXPECT_F(r.z(), 0.0f);
    EXPECT_F(r.w(), 0.0f);

    // Z axis unchanged
    vec4 zAxis(0.0f, 0.0f, 1.0f, 0.0f);
    r = R * zAxis;
    EXPECT_F(r.x(), 0.0f);
    EXPECT_F(r.y(), 0.0f);
    EXPECT_F(r.z(), 1.0f);
    EXPECT_F(r.w(), 0.0f);
}

TEST(Mat4Test, RotatePreservesLength)
{
    // Rotate 60° around the Y axis: quaternion (x=0, y=sin30°, z=0, w=cos30°)
    // quat(x, y, z, w)
    float cos30 = 0.8660254f;
    float sin30 = 0.5f;
    quat  q(0.0f, sin30, 0.0f, cos30);
    mat4  R = mat4::rotate(q);

    // pick an arbitrary direction vector
    vec4 v(1.0f, 2.0f, 3.0f, 0.0f);
    vec4 r = R * v;

    // squared length must be preserved (rigid rotation)
    float lenSqV = v.x() * v.x() + v.y() * v.y() + v.z() * v.z();
    float lenSqR = r.x() * r.x() + r.y() * r.y() + r.z() * r.z();
    EXPECT_F(lenSqR, lenSqV);
}
} // namespace mk::mlm
MK_SIMPLE_MAIN()
