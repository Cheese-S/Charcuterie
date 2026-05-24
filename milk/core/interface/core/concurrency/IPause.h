#pragma once

#include <emmintrin.h>

namespace mk::cc
{
inline void pause()
{
#if defined(__x86_64__) || defined(_M_X64) || defined(__i386__) || defined(_M_IX86)
    _mm_pause();
#elif defined(__arm__) || defined(__aarch64__) || defined(_M_ARM) || defined(_M_ARM64)
    __asm__ volatile("yield");
#else
    // nothing — no-op is safe, just not optimal
#endif
}
} // namespace mk::cc
