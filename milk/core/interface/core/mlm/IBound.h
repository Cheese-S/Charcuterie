#pragma once
#include <core/mlm/IMlm.h>

namespace mk::mlm
{
class Bound
{
public:
    Bound():
        extents_({ kF32Infinity, kF32Infinity, kF32Infinity },
                 { kF32NegInfinity, kF32NegInfinity, kF32NegInfinity })
    {
    }
    Bound(PackedVec3 inMin, PackedVec3 inMax): extents_(inMin, inMax) {}

    void toInclude(PackedVec3 pt);
    void toInclude(const Bound& other);

    bool doesInclude(PackedVec3 pt) const;
    bool doesInclude(const Bound& other) const;

    f32  surfaceArea() const;
    bool empty() const;

    PackedVec3& min()
    {
        return extents_[0];
    }

    PackedVec3& max()
    {
        return extents_[1];
    }

    PackedVec3 min() const
    {
        return extents_[0];
    }

    PackedVec3 max() const
    {
        return extents_[1];
    }

    // Utility for ray bound intersection. Use semantic getter above.
    PackedVec3& operator[](u8 i)
    {
        MK_ASSERT(i <= 2);
        return extents_[i];
    }

    PackedVec3 operator[](u8 i) const
    {
        MK_ASSERT(i <= 2);
        return extents_[i];
    }

private:
    // [0] min, [1] max
    PackedVec3 extents_[2];
};

} // namespace mk::mlm
