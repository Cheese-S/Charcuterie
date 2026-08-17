#pragma once

#include <core/IType.h>
#include <core/IAssert.h>
#include <core/ITraits.h>

#define GLM_FORCE_ALIGNED_GENTYPES
#define GLM_ENABLE_EXPERIMENTAL

#include <glm/glm.hpp>
#include <glm/gtx/norm.hpp>
#include <glm/gtc/type_aligned.hpp>

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

class Quat;

// Wrapper around glm type to ensure consistent behaviors

template<typename T, bool IsAligned>
    requires std::is_arithmetic_v<T>
class Vec2
{
public:
    using DataType = std::conditional_t<IsAligned,
                                        glm::vec<2, T, glm::aligned_highp>,
                                        glm::vec<2, T, glm::packed_highp>>;

    MK_DEFAULT_MOVABLE_DEFAULT_COPYABLE(Vec2);
    Vec2(T xy): data_(xy) {}
    Vec2(T x, T y): data_(x, y) {};
    Vec2(DataType raw): data_(raw) {};

    T& operator[](u8 i);
    T  operator[](u8 i) const;

    T& x();
    T& y();

    T x() const;
    T y() const;

    Vec2 operator-(Vec2 rhs) const;
    Vec2 operator+(Vec2 rhs) const;
    bool operator==(Vec2 rhs) const;

    DataType getRaw() const;

private:
    DataType data_;
};

template<bool IsAligned>
class Vec3
{
    template<bool OtherIsAligned>
    friend class Vec3;

public:
    using DataType = std::conditional_t<IsAligned, glm::aligned_highp_vec3, glm::packed_highp_vec3>;
    MK_DEFAULT_MOVABLE_DEFAULT_COPYABLE(Vec3);
    Vec3(): data_(0) {}
    Vec3(f32 xyz): data_(xyz) {}
    Vec3(f32 x, f32 y, f32 z): data_(x, y, z) {}
    Vec3(DataType raw): data_(raw) {}

    template<bool OtherIsAligned>
    Vec3(Vec3<OtherIsAligned> rhs): data_(rhs.data_)
    {
    }

    f32& x();
    f32& y();
    f32& z();

    f32 x() const;
    f32 y() const;
    f32 z() const;

    f32& operator[](u8 i);
    f32  operator[](u8 i) const;
    Vec3 operator-(Vec3 rhs) const;
    Vec3 operator+(Vec3 rhs) const;
    Vec3 operator/(f32 f) const;
    Vec3 operator/(Vec3 rhs) const;
    bool operator==(Vec3 rhs) const;

    DataType getRaw() const;

private:
    DataType data_;
};

// TODO(Cheese_S): We should convert all static to free functions
template<bool IsAligned>
class Vec4
{
    template<bool OtherIsAligned>
    friend class Vec4;

public:
    using DataType = std::conditional_t<IsAligned, glm::aligned_highp_vec4, glm::packed_highp_vec4>;

    MK_DEFAULT_MOVABLE_DEFAULT_COPYABLE(Vec4);
    explicit Vec4(f32 xyzw): data_(xyzw) {}
    explicit Vec4(DataType data): data_(data) {}
    Vec4(f32 x, f32 y, f32 z, f32 w): data_(x, y, z, w) {}

    template<bool OtherIsAligned>
    Vec4(Vec4<OtherIsAligned> rhs): data_(rhs.data_)
    {
    }

    f32& x();
    f32& y();
    f32& z();
    f32& w();

    f32 x() const;
    f32 y() const;
    f32 z() const;
    f32 w() const;

    f32& operator[](u8 i);
    f32  operator[](u8 i) const;
    Vec4 operator-(Vec4 rhs) const;
    Vec4 operator+(Vec4 rhs) const;
    bool operator==(Vec4 rhs) const;

    friend Vec4 operator*(f32 f, const Vec4& v)
    {
        return Vec4(f * v.data_);
    }

    DataType getRaw() const;

private:
    DataType data_;
};

class Quat
{
public:
    using DataType = glm::aligned_highp_quat;

    MK_DEFAULT_MOVABLE_DEFAULT_COPYABLE(Quat);
    Quat(): data_(1, 0, 0, 0) {};
    Quat(f32 x, f32 y, f32 z, f32 w): data_(w, x, y, z) {}

    f32& x();
    f32& y();
    f32& z();
    f32& w();

    f32 x() const;
    f32 y() const;
    f32 z() const;
    f32 w() const;

    f32& operator[](u8 i);
    f32  operator[](u8 i) const;
    bool operator==(Quat rhs) const;

    DataType getRaw() const;

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
public:
    using Vec4DataType = Vec4<IsAligned>::DataType;
    using DataType = std::conditional_t<IsAligned, glm::aligned_highp_mat4, glm::packed_highp_mat4>;
    MK_DEFAULT_MOVABLE_DEFAULT_COPYABLE(Mat4);
    Mat4(): data_(1.0f) {}

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

    static Mat4 scale(f32 f);
    static Mat4 scale(f32 x, f32 y, f32 z);
    static Mat4 translate(f32 x, f32 y, f32 z);
    static Mat4 inverse(const Mat4& m);
    static Mat4 rotate(Quat q);

    Mat4 operator*(const Mat4& other);

    Vec4<IsAligned> operator*(Vec4<IsAligned> vec) const;

    Vec4DataType& operator[](u8 i);
    Vec4DataType  operator[](u8 i) const;
    bool          operator==(Mat4 rhs) const;

    const DataType& getRaw() const;

private:
    DataType data_;
};

} // namespace mk::mlm::details

namespace mk
{
template<bool IsAligned>
struct IsVec<mlm::details::Vec2<f32, IsAligned>>: std::true_type
{
};

template<bool IsAligned>
struct IsVec<mlm::details::Vec3<IsAligned>>: std::true_type
{
};

template<bool IsAligned>
struct IsVec<mlm::details::Vec4<IsAligned>>: std::true_type
{
};

template<bool IsAligned>
struct IsVec3<mlm::details::Vec3<IsAligned>>: std::true_type
{
};

} // namespace mk

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

using quat = details::Quat;

// Use pakced vectors when serializing
using PackedVec2 = details::Vec2<f32, false>;
using PackedVec3 = details::Vec3<false>;
using PackedVec4 = details::Vec4<false>;
using PackedMat4 = details::Mat4<false>;

static_assert(sizeof(PackedVec2) == 8, "sizeof(PackedVec2) should be equal to 8");
static_assert(sizeof(PackedVec3) == 12, "sizeof(PackedVec3) should be equal to 12");
static_assert(sizeof(PackedVec4) == 16, "sizeof(PackedVec4) should be equal to 16");
static_assert(sizeof(PackedMat4) == 64, "sizeof(PackedMat4) should be equal to 64");

static_assert(sizeof(vec2) == 8, "sizeof(vec2) should be equal to 8");
static_assert(sizeof(vec3) == 16, "sizeof(vec2) should be equal to 16");
static_assert(sizeof(vec4) == 16, "sizeof(vec2) should be equal to 16");
static_assert(sizeof(quat) == 16, "sizeof(quat) should be equal to 16");
static_assert(sizeof(mat4) == 64, "sizeof(mat4) should be equal to 16");

// NOLINTNEXTLINE(modernize-use-std-numbers)
constexpr f32 kPi = 3.14159265358979323846;
// NOLINTNEXTLINE(modernize-use-std-numbers)
constexpr f32 kInvPi = 0.31830988618379067154;   // 1 / pi
constexpr f32 kInv2Pi = 0.15915494309189533577;  // 1 / (2 * pi)
constexpr f32 kInv4Pi = 0.07957747154594766788;  // 1 / (4 * pi)
constexpr f32 kPiOver2 = 1.57079632679489661923; // pi / 2
constexpr f32 kPiOver4 = 0.78539816339744830961; // pi / 4

template<mk::VecType V>
V normalize(V v);
template<mk::VecType V>
f32 dot(V a, V b);
template<mk::VecType V>
f32 distanceSq(V p, V q);
template<mk::VecType V>
f32 distance(V p, V q);
template<mk::VecType V>
f32 magnitudeSq(V v);
template<mk::VecType V>
f32 magnitude(V v);
template<mk::VecType V>
V componentWiseMin(V p, V q);
template<mk::VecType V>
V componentWiseMax(V p, V q);
template<mk::VecType V>
bool anyLessThan(V p, V q);
template<mk::VecType V>
bool anyGreaterThan(V p, V q);
template<mk::VecType V>
bool allLessThan(V p, V q);
template<mk::VecType V>
bool allGreaterThan(V p, V q);
template<mk::VecType V>
bool anyLessEqualThan(V p, V q);
template<mk::VecType V>
bool anyGreaterEqualThan(V p, V q);
template<mk::VecType V>
bool allLessEqualThan(V p, V q);
template<mk::VecType V>
bool allGreaterEqualThan(V p, V q);

template<mk::VecType V>
V operator*(f32 f, V v);
template<mk::VecType V>
V operator/(f32 f, V v);

template<mk::Vec3Type V>
V projectTo(V p, V d);
template<mk::Vec3Type V>
V cross(V p, V q);

PackedVec3 transformPoint(const mat4& m, PackedVec3 v);
PackedVec3 transformVector(const mat4& m, PackedVec3 v);

f32 toFovY(f32 fovX, f32 aspect);

} // namespace mk::mlm

#define MK_MLM_IMPL
#include <core/mlm/IMlm.inl>
#undef MK_MLM_IMPL
