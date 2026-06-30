#include <gtest/gtest.h>
#include <core/math/IMlm.h>

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

//
// ============================================================
// Transform Tests
// ============================================================
//

// TEST(TransformTest, ScaleUniform)
// {
//     Transform t = Transform::scale(2.0f);
//
//     Point p(1, 2, 3);
//     Point r = t * p;
//
//     EXPECT_F(r.x(), 2);
//     EXPECT_F(r.y(), 4);
//     EXPECT_F(r.z(), 6);
// }
//
// TEST(TransformTest, ScaleNonUniform)
// {
//     Transform t = Transform::scale(2.0f, 3.0f, 4.0f);
//
//     Point p(1, 2, 3);
//     Point r = t * p;
//
//     EXPECT_F(r.x(), 2);
//     EXPECT_F(r.y(), 6);
//     EXPECT_F(r.z(), 12);
// }
//
// TEST(TransformTest, TranslatePoint)
// {
//     Transform t = Transform::translate(10, 20, 30);
//
//     Point p(1, 2, 3);
//     Point r = t * p;
//
//     EXPECT_F(r.x(), 11);
//     EXPECT_F(r.y(), 22);
//     EXPECT_F(r.z(), 33);
// }
//
// TEST(TransformTest, TranslateDoesNotAffectDirectionVector)
// {
//     Transform t = Transform::translate(10, 20, 30);
//
//     Vec v(1, 2, 3);
//     Vec r = t * v;
//
//     EXPECT_F(r.x(), 1);
//     EXPECT_F(r.y(), 2);
//     EXPECT_F(r.z(), 3);
// }
//
// TEST(TransformTest, Composition)
// {
//     Transform s = Transform::scale(2, 2, 2);
//     Transform t = Transform::translate(10, 0, 0);
//
//     Transform c = t * s;
//
//     Point p(1, 1, 1);
//     Point r = c * p;
//
//     EXPECT_F(r.x(), 12);
//     EXPECT_F(r.y(), 2);
//     EXPECT_F(r.z(), 2);
// }
//
// TEST(TransformTest, InverseRoundTrip)
// {
//     Transform t = Transform::translate(5, 6, 7) * Transform::scale(2, 3, 4);
//
//     Transform inv = Transform::inverse(t);
//
//     Point p(1, 2, 3);
//
//     Point r = inv * (t * p);
//
//     EXPECT_F(r.x(), 1);
//     EXPECT_F(r.y(), 2);
//     EXPECT_F(r.z(), 3);
// }
//
// TEST(TransformTest, InverseIdentityProperty)
// {
//     Transform t = Transform::translate(3, 4, 5) * Transform::scale(2, 2, 2);
//
//     Transform id = Transform::inverse(t) * t;
//
//     Point p(9, 8, 7);
//     Point r = id * p;
//
//     EXPECT_F(r.x(), 9);
//     EXPECT_F(r.y(), 8);
//     EXPECT_F(r.z(), 7);
// }

} // namespace mk::mlm
