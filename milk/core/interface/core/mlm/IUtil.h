#pragma once
#include <core/mlm/IMlm.h>

namespace mk::mlm
{
class Bound
{
public:
    Bound(): min(0, 0, 0), max(0, 0, 0) {}
    Bound(PackedVec3 inMin, PackedVec3 inMax);

    void include(PackedVec3 pt);

    PackedVec3 min;
    PackedVec3 max;
};

struct Ray
{
    mlm::PackedVec3 o;
    mlm::PackedVec3 d;
};

struct Sphere
{
    mlm::PackedVec3 center;
    f32             r;
};

// ------------------------------- INTERSECTION -------------------------------

bool raySphereIntersection(const Ray& r, const Sphere& s, f32& outT);
bool rayTriIntersection(const Ray& r, vec3 v0, vec3 v1, vec3 v2);

} // namespace mk::mlm
