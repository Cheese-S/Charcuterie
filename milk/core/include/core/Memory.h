#pragma once

#include <core/IType.h>
#include <atomic>
#include <mimalloc.h>

namespace mk::mm::details
{

struct AllocHeader
{
    usize size;
    u32   magic;
};
const usize kHeaderMagic = 0xDCBAABCD;
const usize kScrambledMagic = 0xDEADBEEF;

constexpr usize kAllocHeaderSize = sizeof(AllocHeader);

static_assert(kAllocHeaderSize <= 16, "Must be less than 32 bytes");

struct MemStats
{
    std::atomic<i64> size;
    std::atomic<i64> max;
};

} // namespace mk::mm::details
