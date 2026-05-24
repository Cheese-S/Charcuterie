#pragma once

#include <bit>

namespace mk
{
template<typename T>
constexpr T nextPow2(T val)
{
    return std::bit_ceil(val);
};
} // namespace mk
