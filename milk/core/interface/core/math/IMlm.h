#pragma once

#include <core/IType.h>
#include <core/IAssert.h>
#include <core/ITraits.h>

#define GLM_FORCE_ALIGNED_GENTYPES
#define GLM_ENABLE_EXPERIMENTAL

#include <glm/glm.hpp>
#include <glm/gtx/norm.hpp>
#include <glm/gtc/type_aligned.hpp>

namespace mk::mlm
{
class Transform;
}

namespace mk::mlm::details
{

template<typename T, bool IsAligned>
    requires std::is_arithmetic_v<T>
class Vec2;

template<bool IsAligned>
class Vec3;
template<bool IsAligned>
class Vec4;
template<bool IsAligned>
class Mat4;

// Wrapper around glm type to ensure consistent behaviors

template<typename T, bool IsAligned>
    requires std::is_arithmetic_v<T>
class Vec2
{
    friend class Vec3<IsAligned>;

    friend class Vec4<IsAligned>;

    friend class Mat4<IsAligned>;

    using DataType = std::conditional_t<IsAligned,
                                        glm::vec<2, T, glm::aligned_highp>,
                                        glm::vec<2, T, glm::packed_highp>>;

public:
    MK_DEFAULT_MOVABLE_DEFAULT_COPYABLE(Vec2);
    Vec2(T xy): data_(xy) {}
    Vec2(T x, T y): data_(x, y) {};

    T& operator[](u8 i);

    T& x();
    T& y();

    T x() const;
    T y() const;

private:
    DataType data_;
};

template<bool IsAligned>
class Vec3
{
    friend class Vec2<f32, IsAligned>;
    friend class Vec4<IsAligned>;
    friend class Mat4<IsAligned>;
    friend class ::mk::mlm::Transform;

    using DataType = std::conditional_t<IsAligned, glm::aligned_highp_vec3, glm::packed_highp_vec3>;

public:
    MK_DEFAULT_MOVABLE_DEFAULT_COPYABLE(Vec3);
    Vec3(f32 xyz): data_(xyz) {}
    Vec3(f32 x, f32 y, f32 z): data_(x, y, z) {}

    f32& x();
    f32& y();
    f32& z();

    f32 x() const;
    f32 y() const;
    f32 z() const;

    f32& operator[](u8 i);

private:
    DataType data_;
};

template<bool IsAligned>
class Vec4
{
    friend class Vec2<f32, IsAligned>;
    friend class Vec3<IsAligned>;
    friend class Vec4<IsAligned>;
    friend class Mat4<IsAligned>;
    friend class ::mk::mlm::Transform;

    using DataType = std::conditional_t<IsAligned, glm::aligned_highp_vec4, glm::packed_highp_vec4>;

public:
    MK_DEFAULT_MOVABLE_DEFAULT_COPYABLE(Vec4);
    explicit Vec4(f32 xyzw): data_(xyzw) {}
    explicit Vec4(DataType data): data_(data) {}
    Vec4(f32 x, f32 y, f32 z, f32 w): data_(x, y, z, w) {}

    static Vec4 normalize(const Vec4& vec);
    static f32  dot(const Vec4& a, const Vec4& b);
    static f32  distanceSq(const Vec4& p, const Vec4& q);
    static f32  distance(const Vec4& p, const Vec4& q);
    static f32  magnitudeSq(const Vec4& v);
    static f32  magnitude(const Vec4& v);

    f32& x();
    f32& y();
    f32& z();
    f32& w();

    f32 x() const;
    f32 y() const;
    f32 z() const;
    f32 w() const;

    f32& operator[](u8 i);
    Vec4 operator-(const Vec4& rhs);

    friend Vec4 operator*(f32 f, const Vec4& v)
    {
        return Vec4(f * v.data_);
    }

private:
    DataType data_;
};

// Column Major
// [m00, m10, m20, m30]
// [m01, m11, m21, m31]
// [m02, m12, m22, m32]
// [m03, m13, m23, m33]
template<bool IsAligned>
class Mat4
{
    friend class Vec2<f32, IsAligned>;
    friend class Vec3<IsAligned>;
    friend class Vec4<IsAligned>;
    friend class ::mk::mlm::Transform;

    using DataType = std::conditional_t<IsAligned, glm::aligned_highp_mat4, glm::packed_highp_mat4>;
    using Vec4DataType = Vec4<IsAligned>::DataType;

public:
    MK_DEFAULT_MOVABLE_DEFAULT_COPYABLE(Mat4);
    Mat4(): data_(1.0F) {}

    Mat4(f32 m00,
         f32 m10,
         f32 m20,
         f32 m30,
         f32 m01,
         f32 m11,
         f32 m21,
         f32 m31,
         f32 m02,
         f32 m12,
         f32 m22,
         f32 m32,
         f32 m03,
         f32 m13,
         f32 m23,
         f32 m33):
        data_({ m00, m01, m02, m03 },
              { m10, m11, m12, m13 },
              { m20, m21, m22, m23 },
              { m30, m31, m32, m33 })
    {
    }

    explicit Mat4(DataType mat): data_(mat) {}

    static Mat4 inverse(Mat4 m);

    Mat4 operator*(const Mat4& other);

    Vec4<IsAligned> operator*(const Vec4<IsAligned>& vec);

    Vec4DataType& operator[](u8 i);

private:
    DataType data_;
};

} // namespace mk::mlm::details

// We use column major
namespace mk::mlm
{
// These supports SIMD but also takes up more memory.
using vec2 = details::Vec2<f32, true>;

using u8vec2 = details::Vec2<u8, true>;
using u16vec2 = details::Vec2<u16, true>;
using u32vec2 = details::Vec2<u32, true>;
using u64vec2 = details::Vec2<u64, true>;

using i8vec2 = details::Vec2<i8, true>;
using i16vec2 = details::Vec2<i16, true>;
using i32vec2 = details::Vec2<i32, true>;
using i64vec2 = details::Vec2<i64, true>;

using vec3 = details::Vec3<true>;
using vec4 = details::Vec4<true>;
using mat4 = details::Mat4<true>;

// Use pakced vectors when serializing
using PackedVec2 = details::Vec2<f32, false>;
using PackedVec3 = details::Vec3<false>;
using PackedVec4 = details::Vec4<false>;
using PackedMat4 = details::Mat4<false>;

// NOLINTNEXTLINE(modernize-use-std-numbers)
constexpr f32 kPi = 3.14159265358979323846;
// NOLINTNEXTLINE(modernize-use-std-numbers)
constexpr f32 kInvPi = 0.31830988618379067154;   // 1 / pi
constexpr f32 kInv2Pi = 0.15915494309189533577;  // 1 / (2 * pi)
constexpr f32 kInv4Pi = 0.07957747154594766788;  // 1 / (4 * pi)
constexpr f32 kPiOver2 = 1.57079632679489661923; // pi / 2
constexpr f32 kPiOver4 = 0.78539816339744830961; // pi / 4

class Point;
class Vector;

class Transform
{
public:
    Transform() = default;
    explicit Transform(mat4 mat): data_(mat) {};

    static Transform scale(f32 f);
    static Transform scale(f32 x, f32 y, f32 z);

    static Transform translate(f32 x, f32 y, f32 z);

    static Transform inverse(const Transform& t);

    Transform operator*(const Transform& other);
    Point     operator*(const Point& pt);
    Vector    operator*(const Vector& vec);

private:
    mat4 data_;
};

class Point
{
    friend Transform;

public:
    Point(f32 x, f32 y, f32 z): data_(x, y, z, 1) {}
    // TODO(Cheese_S): Maybe just force set w to 1
    Point(vec4 vec): data_(vec)
    {
        MK_ASSERT(vec.w() == 1);
    }

    explicit Point(const Vector& v);

    f32& x();
    f32& y();
    f32& z();

    f32 x() const;
    f32 y() const;
    f32 z() const;

    Vector operator-(const Point& rhs);

    friend f32 distanceSq(const Point& p, const Point& q);
    friend f32 distance(const Point& p, const Point& q);

private:
    vec4 data_;
};

class Vector
{
    friend Transform;

public:
    static Vector normalize(const Vector& vec);
    static f32    dot(const Vector& p, const Vector& q);
    static f32    magnitudeSq(const Vector& v);
    static f32    magnitude(const Vector& v);

    Vector(f32 x, f32 y, f32 z): data_(x, y, z, 0) {}
    // TODO(Cheese_S): Maybe just force set w to 0
    Vector(vec4 vec): data_(vec)
    {
        MK_ASSERT(vec.w() == 0);
    }

    explicit Vector(const Point& pt): Vector(pt.x(), pt.y(), pt.z()) {}

    f32& x();
    f32& y();
    f32& z();

    f32 x() const;
    f32 y() const;
    f32 z() const;

    friend Vector operator*(f32 d, const Vector& v);

private:
    vec4 data_;
};

struct Ray
{
    mlm::Point  o;
    mlm::Vector d;
};

struct Sphere
{
    mlm::Point center;
    f32        r;
};

struct Bound2d
{
    vec2 min;
    vec2 max;
};

Point projectTo(const Point& p, const Vector& d);
f32   toFovY(f32 fovX, f32 aspect);

// ------------------------------- INTERSECTION -------------------------------

bool raySphereIntersection(const Ray& r, const Sphere& s, f32& outT);

} // namespace mk::mlm

#define MK_MLM_IMPL
#include <core/math/IMlm.inl>
#undef MK_MLM_IMPL
