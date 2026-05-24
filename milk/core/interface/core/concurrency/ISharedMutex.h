#pragma once
#include <atomic>
#include <core/IType.h>
#include <core/IMacro.h>
#include <core/util/IClock.h>

namespace mk::cc
{

class SharedMutex
{
    // states
    static constexpr u32 kELockedBit = 1 << 0;
    static constexpr u32 kEParkedBit = 1 << 1;
    static constexpr u32 kEDrainBit = 1 << 2;
    static constexpr u32 kEMask = kELockedBit | kEParkedBit;

    static constexpr u32 kSIncr = 1 << 11;
    static constexpr u32 kSParkedBit = 1 << 3;
    static constexpr u32 kSCountMask = ~(kSIncr - 1);

    static constexpr u32 kMaxSpinCount = 40;

public:
    MK_NON_MOVABLE_NON_COPYABLE(SharedMutex);
    SharedMutex() = default;

    void               lock();
    [[nodiscard]] bool tryLock(util::Duration duration = util::kImmediate);

    void               lockShared();
    [[nodiscard]] bool tryLockShared(util::Duration duration = util::kImmediate);

    void unlock();
    void unlockShared();

private:
    bool tryLockImpl(util::Duration duration);
    bool tryLockSharedImpl(util::Duration duration);

    void drainShared();

    std::atomic<u32> state_;
};

} // namespace mk::cc
