#pragma once
#include <core/mlm/IMlm.h>
#include <core/mlm/IBound.h>
#include <core/container/IVector.h>

namespace mk::mlm
{
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

// ------------------------------ TRANSFORM ------------------------------- //

Ray transformRay(const mat4& m, const Ray& r);

// ------------------------------- INTERSECTION -------------------------------

bool raySphereIntersection(const Ray& r, const Sphere& s, f32& outT);
bool rayTriIntersection(const Ray& r, f32 tMax, vec3 v0, vec3 v1, vec3 v2);

// dirIsNeg[x] can only be 0 or 1;
bool rayBoundIntersection(const Ray&     r,
                          const Bound&   bound,
                          f32            rtMax,
                          PackedVec3     invD,
                          VectorView<u8> dirIsNeg);

} // namespace mk::mlm
