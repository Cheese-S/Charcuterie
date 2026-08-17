#pragma once

#include <cstddef>
#include <cstdint>
#include <cmath>

namespace mk
{
using u64 = std::uint64_t;
using u32 = std::uint32_t;
using u16 = std::uint16_t;
using u8 = std::uint8_t;

using i64 = std::int64_t;
using i32 = std::int32_t;
using i16 = std::int16_t;
using i8 = std::int8_t;

using f64 = double;
using f32 = float;

using isize = std::ptrdiff_t;
using usize = std::size_t;

using uptr = std::uintptr_t;
using iptr = std::intptr_t;

using byte = std::byte;

using nullptr_t = decltype(nullptr);

static constexpr u8    kU8Max = UINT8_MAX;
static constexpr u16   kU16Max = UINT16_MAX;
static constexpr u32   kU32Max = UINT32_MAX;
static constexpr u64   kU64Max = UINT64_MAX;
static constexpr usize kUsizeMax = SIZE_MAX;

static constexpr i8    kI8Max = INT8_MAX;
static constexpr i16   kI16Max = INT16_MAX;
static constexpr i32   kI32Max = INT32_MAX;
static constexpr i64   kI64Max = INT64_MAX;
static constexpr usize kIsizeMax = PTRDIFF_MAX;

static constexpr i8    kI8Min = INT8_MIN;
static constexpr i16   kI16Min = INT16_MIN;
static constexpr i32   kI32Min = INT32_MIN;
static constexpr i64   kI64Min = INT64_MIN;
static constexpr usize kIsizeMin = PTRDIFF_MIN;

static constexpr f32 kF32Infinity = INFINITY;
static constexpr f32 kF32NegInfinity = -INFINITY;

}; // namespace mk
