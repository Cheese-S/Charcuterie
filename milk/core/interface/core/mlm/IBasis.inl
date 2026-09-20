#pragma once

#include <cmath>

#ifndef MK_BASIS_IMPL
    #include <core/mlm/IBasis.h>
#endif

namespace mk::mlm
{
template<mk::Vec3Type V>
Basis Basis::xy(V x, V y)
{
    MK_ASSERT(isNormalized(x, kEpislon) && isNormalized(y, kEpislon));

    return Basis(x, y, mlm::cross(y, x));
}

template<mk::Vec3Type V>
Basis Basis::xz(V x, V z)
{
    MK_ASSERT(isNormalized(x, kEpislon) && isNormalized(z, kEpislon));

    return Basis(x, mlm::cross(x, z), z);
}

template<mk::Vec3Type V>
Basis Basis::yz(V y, V z)
{
    MK_ASSERT(isNormalized(y, kEpislon) && isNormalized(z, kEpislon));

    return Basis(mlm::cross(z, y), y, z);
}

template<mk::VecType V>
Basis Basis::x(V x)
{
    V t;
    V b;
    makeBasis(x, t, b);
    return Basis(x, b, t);
}

template<mk::VecType V>
Basis Basis::y(V y)
{
    V t;
    V b;
    makeBasis(y, t, b);
    return Basis(t, y, b);
}

template<mk::VecType V>
Basis Basis::z(V z)
{
    V t;
    V b;
    makeBasis(z, t, b);
    return Basis(b, t, z);
}

template<mk::Vec3Type V>
void Basis::makeBasis(V n, V& outT, V& outB)
{
    f32 sign = std::copysign(1, n.z());
    f32 a = -1 / (sign + n.z());
    f32 b = n.x() * n.y() * a;
    outT = V(1 + sign * n.x() * n.x() * a, sign * b, -sign * n.x());
    outB = V(b, sign + n.y() * n.y() * a, -n.y());
}

} // namespace mk::mlm
