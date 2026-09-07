#include <core/mlm/IBound.h>
namespace mk::mlm
{
// -------------------------------- BOUND --------------------------------- //
void Bound::toInclude(PackedVec3 pt)
{
    min() = mlm::componentWiseMin(pt, min());
    max() = mlm::componentWiseMax(pt, max());
}

void Bound::toInclude(const Bound& other)
{
    min() = mlm::componentWiseMin(other.min(), min());
    max() = mlm::componentWiseMax(other.max(), max());
}

f32 Bound::surfaceArea() const
{
    PackedVec3 extent = max() - min();
    return 2 * (extent.x() * extent.y() + extent.x() * extent.z() + extent.y() * extent.z());
}

bool Bound::empty() const
{
    return anyLessThan<PackedVec3>(max(), min());
}

bool Bound::doesInclude(PackedVec3 pt) const
{
    return mlm::allGreaterEqualThan(pt, min()) && mlm::allLessEqualThan(pt, max());
}

bool Bound::doesInclude(const Bound& other) const
{
    return mlm::allLessEqualThan(min(), other.min()) &&
           mlm::allGreaterEqualThan(max(), other.max());
}

} // namespace mk::mlm
