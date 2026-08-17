#ifndef MK_MLM_IMPL
    #include <core/mlm/IMlm.h>
#endif

namespace mk::mlm::details
{
// ----------------------------------- Vec2 -----------------------------------
template<typename T, bool IsAligned>
    requires std::is_arithmetic_v<T>
T& Vec2<T, IsAligned>::operator[](u8 i)
{
    MK_ASSERT(i < 2);
    return data_[i];
}

template<typename T, bool IsAligned>
    requires std::is_arithmetic_v<T>
T Vec2<T, IsAligned>::operator[](u8 i) const
{
    MK_ASSERT(i < 2);
    return data_[i];
}

template<typename T, bool IsAligned>
    requires std::is_arithmetic_v<T>
T& Vec2<T, IsAligned>::x()
{
    return data_.x;
}

template<typename T, bool IsAligned>
    requires std::is_arithmetic_v<T>
T& Vec2<T, IsAligned>::y()
{
    return data_.y;
}

template<typename T, bool IsAligned>
    requires std::is_arithmetic_v<T>
T Vec2<T, IsAligned>::x() const
{
    return data_.x;
}

template<typename T, bool IsAligned>
    requires std::is_arithmetic_v<T>
T Vec2<T, IsAligned>::y() const
{
    return data_.y;
}

template<typename T, bool IsAligned>
    requires std::is_arithmetic_v<T>
Vec2<T, IsAligned> Vec2<T, IsAligned>::operator-(Vec2<T, IsAligned> rhs) const
{
    return data_ - rhs.data_;
}

template<typename T, bool IsAligned>
    requires std::is_arithmetic_v<T>
Vec2<T, IsAligned> Vec2<T, IsAligned>::operator+(Vec2<T, IsAligned> rhs) const
{
    return data_ + rhs.data_;
}

template<typename T, bool IsAligned>
    requires std::is_arithmetic_v<T>
bool Vec2<T, IsAligned>::operator==(Vec2<T, IsAligned> rhs) const
{
    return getRaw() == rhs.getRaw();
}

template<typename T, bool IsAligned>
    requires std::is_arithmetic_v<T>
Vec2<T, IsAligned>::DataType Vec2<T, IsAligned>::getRaw() const
{
    return data_;
}

// ----------------------------------- Vec3 -----------------------------------

template<bool IsAligned>
f32& Vec3<IsAligned>::operator[](u8 i)
{
    MK_ASSERT(i < 3);
    return data_[i];
}

template<bool IsAligned>
f32 Vec3<IsAligned>::operator[](u8 i) const
{
    MK_ASSERT(i < 3);
    return data_[i];
}

template<bool IsAligned>
f32& Vec3<IsAligned>::x()
{
    return data_.x;
}

template<bool IsAligned>
f32& Vec3<IsAligned>::y()
{
    return data_.y;
}

template<bool IsAligned>
f32& Vec3<IsAligned>::z()
{
    return data_.z;
}

template<bool IsAligned>
f32 Vec3<IsAligned>::x() const
{
    return data_.x;
}

template<bool IsAligned>
f32 Vec3<IsAligned>::y() const
{
    return data_.y;
}

template<bool IsAligned>
f32 Vec3<IsAligned>::z() const
{
    return data_.z;
}

template<bool IsAligned>
Vec3<IsAligned> Vec3<IsAligned>::operator-(Vec3<IsAligned> rhs) const
{
    return getRaw() - rhs.getRaw();
}

template<bool IsAligned>
Vec3<IsAligned> Vec3<IsAligned>::operator+(Vec3<IsAligned> rhs) const
{
    return Vec3<IsAligned>(getRaw() + rhs.getRaw());
}

template<bool IsAligned>
Vec3<IsAligned> Vec3<IsAligned>::operator/(f32 f) const
{
    return getRaw() / f;
}

template<bool IsAligned>
Vec3<IsAligned> Vec3<IsAligned>::operator/(Vec3<IsAligned> rhs) const
{
    return getRaw() / rhs.getRaw();
}

template<bool IsAligned>
bool Vec3<IsAligned>::operator==(Vec3<IsAligned> rhs) const
{
    return getRaw() == rhs.getRaw();
}

template<bool IsAligned>
Vec3<IsAligned>::DataType Vec3<IsAligned>::getRaw() const
{
    return data_;
}

// ----------------------------------- Vec4 -----------------------------------

template<bool IsAligned>
f32& Vec4<IsAligned>::operator[](u8 i)
{
    MK_ASSERT(i >= 0 && i < 4);
    return data_[i];
}

template<bool IsAligned>
f32& Vec4<IsAligned>::x()
{
    return data_.x;
}

template<bool IsAligned>
f32& Vec4<IsAligned>::y()
{
    return data_.y;
}

template<bool IsAligned>
f32& Vec4<IsAligned>::z()
{
    return data_.z;
}

template<bool IsAligned>
f32& Vec4<IsAligned>::w()
{
    return data_.w;
}

template<bool IsAligned>
f32 Vec4<IsAligned>::x() const
{
    return data_.x;
}

template<bool IsAligned>
f32 Vec4<IsAligned>::y() const
{
    return data_.y;
}

template<bool IsAligned>
f32 Vec4<IsAligned>::z() const
{
    return data_.z;
}

template<bool IsAligned>
f32 Vec4<IsAligned>::w() const
{
    return data_.w;
}

template<bool IsAligned>
Vec4<IsAligned> Vec4<IsAligned>::operator-(Vec4<IsAligned> rhs) const
{
    return data_ - rhs.data_;
}

template<bool IsAligned>
Vec4<IsAligned> Vec4<IsAligned>::operator+(Vec4<IsAligned> rhs) const
{
    return data_ + rhs.data_;
}

template<bool IsAligned>
bool Vec4<IsAligned>::operator==(Vec4<IsAligned> rhs) const
{
    return getRaw() == rhs.getRaw();
}

template<bool IsAligned>
Vec4<IsAligned>::DataType Vec4<IsAligned>::getRaw() const
{
    return data_;
}

// ----------------------------------- Mat4 -----------------------------------
template<bool IsAligned>
Mat4<IsAligned> Mat4<IsAligned>::inverse(const Mat4<IsAligned>& m)
{
    return Mat4<IsAligned>(glm::inverse(m.getRaw()));
}

template<bool IsAligned>
Mat4<IsAligned> Mat4<IsAligned>::operator*(const Mat4<IsAligned>& other)
{
    return Mat4{ this->data_ * other.data_ };
}

template<bool IsAligned>
Vec4<IsAligned> Mat4<IsAligned>::operator*(Vec4<IsAligned> vec) const
{
    return Vec4<IsAligned>(this->data_ * vec.getRaw());
}

template<bool IsAligned>
Mat4<IsAligned>::Vec4DataType& Mat4<IsAligned>::operator[](u8 i)
{
    MK_ASSERT(i < 4);
    return data_[i];
}

template<bool IsAligned>
Mat4<IsAligned>::Vec4DataType Mat4<IsAligned>::operator[](u8 i) const
{
    MK_ASSERT(i < 4);
    return data_[i];
}

template<bool IsAligned>
bool Mat4<IsAligned>::operator==(Mat4<IsAligned> rhs) const
{
    return getRaw() == rhs.getRaw();
}

template<bool IsAligned>
Mat4<IsAligned> Mat4<IsAligned>::scale(f32 f)
{
    return Mat4<IsAligned>(glm::scale(Mat4<IsAligned>::DataType(1.0f), { f, f, f }));
}

template<bool IsAligned>
Mat4<IsAligned> Mat4<IsAligned>::scale(f32 x, f32 y, f32 z)
{
    return Mat4<IsAligned>(glm::scale(Mat4<IsAligned>::DataType(1.0f), { x, y, z }));
}

template<bool IsAligned>
Mat4<IsAligned> Mat4<IsAligned>::translate(f32 x, f32 y, f32 z)
{
    return Mat4<IsAligned>(glm::translate(Mat4<IsAligned>::DataType(1.0f), { x, y, z }));
}

template<bool IsAligned>
Mat4<IsAligned> Mat4<IsAligned>::rotate(quat q)
{
    return Mat4<IsAligned>(glm::mat4_cast(q.getRaw()));
}

template<bool IsAligned>
const Mat4<IsAligned>::DataType& Mat4<IsAligned>::getRaw() const
{
    return data_;
}

} // namespace mk::mlm::details

namespace mk::mlm
{
template<mk::VecType V>
V normalize(V v)
{
    return V(glm::normalize(v.getRaw()));
}

template<mk::VecType V>
f32 dot(V a, V b)
{
    return glm::dot(a.getRaw(), b.getRaw());
}

template<mk::VecType V>
f32 distanceSq(V p, V q)
{
    return glm::distance2(p.getRaw(), q.getRaw());
}

template<mk::VecType V>
f32 distance(V p, V q)
{
    return glm::distance(p.getRaw(), q.getRaw());
}

template<mk::VecType V>
f32 magnitudeSq(V v)
{
    return glm::length2(v.getRaw());
}

template<mk::VecType V>
f32 magnitude(V v)
{
    return glm::length(v.getRaw());
}

template<mk::VecType V>
V componentWiseMin(V p, V q)
{
    return glm::min(p.getRaw(), q.getRaw());
}

template<mk::VecType V>
V componentWiseMax(V p, V q)
{
    return glm::max(p.getRaw(), q.getRaw());
}

template<mk::VecType V>
bool anyLessThan(V p, V q)
{
    return glm::any(glm::lessThan(p.getRaw(), q.getRaw()));
}

template<mk::VecType V>
bool anyGreaterThan(V p, V q)
{
    return glm::any(glm::greaterThan(p.getRaw(), q.getRaw()));
}

template<mk::VecType V>
bool anyLessEqualThan(V p, V q)
{
    return glm::any(glm::lessThanEqual(p.getRaw(), q.getRaw()));
}

template<mk::VecType V>
bool anyGreaterEqualThan(V p, V q)
{
    return glm::any(glm::greaterThanEqual(p.getRaw(), q.getRaw()));
}

template<mk::VecType V>
bool allLessThan(V p, V q)
{
    return glm::all(glm::lessThan(p.getRaw(), q.getRaw()));
}

template<mk::VecType V>
bool allGreaterThan(V p, V q)
{
    return glm::all(glm::greaterThan(p.getRaw(), q.getRaw()));
}

template<mk::VecType V>
bool allLessEqualThan(V p, V q)
{
    return glm::all(glm::lessThanEqual(p.getRaw(), q.getRaw()));
}

template<mk::VecType V>
bool allGreaterEqualThan(V p, V q)
{
    return glm::all(glm::greaterThanEqual(p.getRaw(), q.getRaw()));
}

template<mk::VecType V>
V operator*(f32 f, V v)
{
    return V(f * v.getRaw());
}

template<mk::Vec3Type V>
V cross(V p, V q)
{
    return glm::cross(p.getRaw(), q.getRaw());
}

} // namespace mk::mlm
