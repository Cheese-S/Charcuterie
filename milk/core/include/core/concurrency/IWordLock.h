#pragma once
#include <core/IType.h>
#include <atomic>

namespace mk::cc
{
class WordLock
{
    static const u8 kLockedBit = 1 << 0;
    static const u8 kQueueLockedBit = 1 << 1;
    static const u8 kAddrMask = kLockedBit | kQueueLockedBit;

public:
    void lock()
    {
        uintptr_t expected = 0;
        if (addr_.compare_exchange_weak(expected, WordLock::kLockedBit))
        {
            return;
        }

        lockSlow();
    }

    void unlock()
    {
        uintptr_t expected = WordLock::kLockedBit;
        if (addr_.compare_exchange_weak(expected, 0))
        {
            return;
        }

        unlockSlow();
    }

private:
    void lockSlow();
    void unlockSlow();

    std::atomic<uptr> addr_;
};
} // namespace mk::cc
