#pragma once
#include <core/IMacro.h>
#include <core/concurrency/ILockTraits.h>
#include <core/concurrency/IScopedLock.h>
#include <core/concurrency/IParkingLot.h>

namespace mk::cc
{

template<typename T>
    requires IsLockable<T>
class Condition
{
    static constexpr u32 kMask = 0xFFFFFFFF;

public:
    MK_NON_MOVABLE_NON_COPYABLE(Condition);

    Condition(): hasWaiters_(false) {};
    ~Condition() = default;

    // Returns eOk when waken up by another thread
    // Returns eTimeout when timed out.
    Result wait(T& lock, util::Duration duration = util::kForever);
    // Simialr to c++ cv works. The user can hold / not hold the lock
    // while calling the notify functions
    void   notifyOne();
    void   notifyAll();

private:
    std::atomic<bool> hasWaiters_;
};

} // namespace mk::cc
#define MK_CONDITION_IMPL
#include <core/concurrency/ICondition.inl>
#undef MK_CONDITION_IMPL
