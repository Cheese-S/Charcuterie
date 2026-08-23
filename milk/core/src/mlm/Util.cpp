#include <core/mlm/IUtil.h>
#include <core/mlm/IFormatter.h>
#include <core/log/IRawLog.h>
#include <algorithm>
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

// ------------------------------ INTERSECT ------------------------------- //

bool raySphereIntersection(const Ray& r, const Sphere& s, f32& outT)
{
    f32        mag = dot(s.center, r.d);
    PackedVec3 projected = mag * r.d;
    bool       res = distanceSq(projected, s.center) <= s.r * s.r;

    if (res)
    {
        outT = mag;
    }
    return res;
}

// TODO(Cheese_S): micro benchmark to see if it's worth to use simd
// Or, we should just keep it as Vec3s.
bool rayTriIntersection(const Ray& r, f32 tMax, vec3 v0, vec3 v1, vec3 v2)
{
    constexpr f32 kEpislon = std::numeric_limits<f32>::epsilon();
    vec3          o = vec3(r.o);
    vec3          d = vec3(r.d);

    vec3 e1 = v1 - v0;
    vec3 e2 = v2 - v0;

    const vec3 n = cross(e1, e2);
    if (dot(d, n) > 0)
    {
        return false;
    }

    vec3 rayCrossE2 = cross(d, e2);
    f32  det = dot(e1, rayCrossE2);

    if (abs(det) < kEpislon)
    {
        return false;
    }

    f32  invDet = 1.0f / det;
    vec3 s = o - v0;
    f32  u = dot(s, rayCrossE2) * invDet;

    if (u < -kEpislon || u > kEpislon + 1)
    {
        return false;
    }

    vec3 sCrossE1 = cross(s, e1);
    f32  v = dot(d, sCrossE1) * invDet;

    if (v < -kEpislon || u + v - 1 > kEpislon)
    {
        return false;
    }

    f32 t = invDet * dot(e2, sCrossE1);

    return t > kEpislon && t < tMax;
}

bool rayBoundIntersection(const Ray&     r,
                          const Bound&   bound,
                          f32            rtMax,
                          PackedVec3     invD,
                          VectorView<u8> dirIsNeg)
{
    MK_ASSERT(dirIsNeg[0] <= 2 && dirIsNeg[1] <= 2 && dirIsNeg[2] <= 2);

    f32 tMin = (bound[dirIsNeg[0]].x() - r.o.x()) * invD.x();
    f32 tMax = (bound[1 - dirIsNeg[0]].x() - r.o.x()) * invD.x();
    f32 tyMin = (bound[dirIsNeg[1]].y() - r.o.y()) * invD.y();
    f32 tyMax = (bound[1 - dirIsNeg[1]].y() - r.o.y()) * invD.y();
    if (tMin > tyMax || tyMin > tMax)
    {
        return false;
    }

    tMin = std::max(tyMin, tMin);
    tMax = std::min(tyMax, tMax);

    f32 tzMin = (bound[dirIsNeg[2]].z() - r.o.z()) * invD.z();
    f32 tzMax = (bound[1 - dirIsNeg[2]].z() - r.o.z()) * invD.z();

    if (tMin > tzMax || tzMin > tMax)
    {
        return false;
    }

    tMin = std::max(tzMin, tMin);
    tMax = std::min(tzMax, tMax);

    return tMin < rtMax && tMax > 0;
}

Ray transformRay(const mat4& m, const Ray& r)
{
    return Ray{
        .o = transformPoint(m, r.o),
        .d = normalize(transformVector(m, r.d)),
    };
}

} // namespace mk::mlm
