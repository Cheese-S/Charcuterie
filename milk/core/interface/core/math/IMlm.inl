#ifndef MK_MLM_IMPL
    #include <core/math/IMlm.h>
#endif

namespace mk::mlm::details
{
// ----------------------------------- Vec2 -----------------------------------
template<typename T, bool IsAligned>
    requires std::is_arithmetic_v<T>
T& Vec2<T, IsAligned>::operator[](u8 i)
{
    MK_ASSERT(i >= 0 && i < 2);
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

// ----------------------------------- Vec3 -----------------------------------
template<bool IsAligned>
Vec3<IsAligned> Vec3<IsAligned>::cross(Vec3<IsAligned> p, Vec3<IsAligned> q)
{
    return glm::cross(p.data_, q.data_);
}

template<bool IsAligned>
f32& Vec3<IsAligned>::operator[](u8 i)
{
    MK_ASSERT(i >= 0 && i < 3);
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
Vec3<IsAligned> Vec3<IsAligned>::operator-(const Vec3<IsAligned>& rhs)
{
    return Vec3<IsAligned>(data_ - rhs.data_);
}

// ----------------------------------- Vec4 -----------------------------------

template<bool IsAligned>
Vec4<IsAligned> Vec4<IsAligned>::normalize(Vec4<IsAligned> vec)
{
    return Vec4<IsAligned>(glm::normalize(vec.data_));
}

template<bool IsAligned>
f32 Vec4<IsAligned>::dot(Vec4<IsAligned> a, Vec4<IsAligned> b)
{
    return glm::dot(a.data_, b.data_);
}

template<bool IsAligned>
f32 Vec4<IsAligned>::distanceSq(Vec4<IsAligned> p, Vec4<IsAligned> q)
{
    return glm::distance2(p.data_, q.data_);
}

template<bool IsAligned>
f32 Vec4<IsAligned>::distance(Vec4<IsAligned> p, Vec4<IsAligned> q)
{
    return glm::distance(p.data_, q.data_);
}

template<bool IsAligned>
f32 Vec4<IsAligned>::magnitudeSq(Vec4<IsAligned> v)
{
    return glm::length2(v.data_);
}

template<bool IsAligned>
f32 Vec4<IsAligned>::magnitude(Vec4<IsAligned> v)
{
    return glm::length(v.data_);
}

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
    return Vec4<IsAligned>(data_ - rhs.data_);
}

// ----------------------------------- Mat4 -----------------------------------
template<bool IsAligned>
Mat4<IsAligned> Mat4<IsAligned>::inverse(Mat4<IsAligned> m)
{
    return Mat4<IsAligned>(glm::inverse(m.data_));
}

template<bool IsAligned>
Mat4<IsAligned> Mat4<IsAligned>::operator*(const Mat4<IsAligned>& other)
{
    return Mat4{ this->data_ * other.data_ };
}

template<bool IsAligned>
Vec4<IsAligned> Mat4<IsAligned>::operator*(const Vec4<IsAligned>& vec)
{
    return Vec4<IsAligned>(this->data_ * vec.data_);
}

template<bool IsAligned>
Mat4<IsAligned>::Vec4DataType& Mat4<IsAligned>::operator[](u8 i)
{
    MK_ASSERT(i >= 0 && i < 4);
    return data_[i];
}

} // namespace mk::mlm::details
