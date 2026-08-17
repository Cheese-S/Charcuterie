#include <core/mlm/IMlm.h>

namespace mk::mlm
{

// --------------------------------- Quat --------------------------------- //
namespace details
{
f32& Quat::x()
{
    return data_.x;
}

f32& Quat::y()
{
    return data_.y;
}

f32& Quat::z()
{
    return data_.z;
}

f32& Quat::w()
{
    return data_.w;
}

f32 Quat::x() const
{
    return data_.x;
}

f32 Quat::y() const
{
    return data_.y;
}

f32 Quat::z() const
{
    return data_.z;
}

f32 Quat::w() const
{
    return data_.w;
}

Quat::DataType Quat::getRaw() const
{
    return data_;
}

f32& Quat::operator[](u8 i)
{
    MK_ASSERT(i < 4);
    return data_[i];
}

f32 Quat::operator[](u8 i) const
{
    MK_ASSERT(i < 4);
    return data_[i];
}

} // namespace details

// --------------------------------- util --------------------------------- //

PackedVec3 transformPoint(const mat4& m, PackedVec3 v)
{
    vec4 pt = m * vec4(v.x(), v.y(), v.z(), 1);
    if (pt.w() != 1.0f)
    {
        pt = (1 / pt.w()) * pt;
    }
    return PackedVec3(pt.x(), pt.y(), pt.z());
}

PackedVec3 transformVector(const mat4& m, PackedVec3 v)
{
    vec4 vec = m * vec4(v.x(), v.y(), v.z(), 0);
    return PackedVec3(vec.x(), vec.y(), vec.z());
}

f32 toFovY(f32 fovX, f32 aspect)
{
    return 2.0f * std::atan(std::tan(fovX * 0.5f) / aspect);
}

} // namespace mk::mlm
