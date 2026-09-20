#pragma once

#include <core/mlm/IMlm.h>
#include <core/mlm/ITraits.h>

namespace mk::mlm
{
class Basis
{
public:
    template<mk::Vec3Type V>
    static Basis xy(V x, V y);

    template<mk::Vec3Type V>
    static Basis xz(V x, V z);

    template<mk::Vec3Type V>
    static Basis yz(V y, V z);

    template<mk::VecType V>
    static Basis x(V x);

    template<mk::VecType V>
    static Basis y(V y);

    template<mk::VecType V>
    static Basis z(V z);

    template<mk::Vec3Type V>
    Basis(V x, V y, V z): x_(x), y_(y), z_(z)
    {
    }

    PackedVec3 x() const
    {
        return x_;
    }

    PackedVec3 y() const
    {
        return y_;
    }

    PackedVec3 z() const
    {
        return z_;
    }

private:
    template<mk::Vec3Type V>
    static void makeBasis(V n, V& outT, V& outB);

    PackedVec3 x_;
    PackedVec3 y_;
    PackedVec3 z_;
};
} // namespace mk::mlm

#define MK_BASIS_IMPL
#include <core/mlm/IBasis.inl>
#undef MK_BASIS_IMPL
