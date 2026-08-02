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
} // namespace details

// --------------------------------- util --------------------------------- //

f32 toFovY(f32 fovX, f32 aspect)
{
    return 2.0f * std::atan(std::tan(fovX * 0.5f) / aspect);
}

} // namespace mk::mlm
