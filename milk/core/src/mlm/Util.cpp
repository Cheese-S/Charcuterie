#include <core/mlm/IUtil.h>
namespace mk::mlm
{
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
bool rayTriIntersection(const Ray& r, vec3 v0, vec3 v1, vec3 v2)
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

    // TODO(Cheese_S): missing intersection time
    return v >= -kEpislon && u + v - 1 <= kEpislon;
    // if (v < -kEpislon || u + v - 1 > kEpislon)
    // {
    //     return false;
    // }
    //
    // return true;
}
} // namespace mk::mlm
