#include <core/concurrency/ISharedMutex.h>
#include <core/concurrency/IParkingLot.h>
#include <core/IAtomic.h>
#include <core/log/IRawLog.h>
#include <fmt/std.h>

namespace mk::cc
{

// TODO(Cheese_S): deferred reader optimization
// TODO(Cheese_S): upgradbale lock
//      Basic Idea: Add UParkedBit and ULockedBit
//          Writers -> Block getting ELockedBit on ULockedBit (? DO we need ULockedBit or
//          can we just reuse ELockedBit)
//                  -> Drain both S and U (since U can set U bit while holding S bit, E
//                  can neven win)
//          Readers -> Block increase SCount on ULockedBit && UParkedBit
//          Upgradable -> Block on UParkedBit, i.e. only one upgradbale lock at a time.
//                     -> Unlock U is basically unlock
//          Goal -> when a lock gets upgraded, the underlying state must not be mutated by
//          another writer
//

void SharedMutex::lock()
{
    tryLockImpl(util::kForever);
}

bool SharedMutex::tryLock(util::Duration duration)
{
    return tryLockImpl(duration);
}

void SharedMutex::unlock()
{
    u32 current;
    for (;;)
    {
        current = state_.load(std::memory_order_relaxed);
        if (current == kELockedBit)
        {
            if (!compareExchangeWeak(state_, current, static_cast<u32>(0)))
            {
                continue;
            }
            return;
        }

        u32 afterMask = (kEParkedBit | kSParkedBit | kELockedBit);
        if (current & kEParkedBit)
        {
            bool didUnparkOne = false;
            afterMask = afterMask & ~kELockedBit;
            ParkingLot::unparkOne(this,
                                  kEParkedBit,
                                  [&](UnparkResult result)
                                  {
                                      if (result != UnparkResult::eMayHaveMore)
                                      {
                                          // BUG HERE: eMayHaveMore may indicate waiting
                                          // readers not always writers
                                          state_ &= ~(kELockedBit | kEParkedBit);
                                          didUnparkOne = true;
                                      }
                                      else
                                      {
                                          state_ &= ~(kELockedBit);
                                      }
                                      didUnparkOne = result != UnparkResult::eNotFound;
                                  });

            if (didUnparkOne)
            {
                return;
            }
        }

        state_ &= ~(afterMask);
        ParkingLot::unparkAll(this, kSParkedBit | kEParkedBit);
        return;
    }
}

void SharedMutex::lockShared()
{
    tryLockSharedImpl(util::kForever);
}

bool SharedMutex::tryLockShared(util::Duration duration)
{
    return tryLockSharedImpl(duration);
}

void SharedMutex::unlockShared()
{
    u32 current;
    for (;;)
    {
        current = state_.load(std::memory_order_relaxed);
        MK_ASSERT(current & kSCountMask);
        if (!compareExchangeWeak(state_, current, current - kSIncr))
        {
            continue;
        }

        if (!(current & kEDrainBit))
        {
            return;
        }

        if ((current & kSCountMask) == kSIncr)
        {
            ParkingLot::unparkOne(this,
                                  kEDrainBit,
                                  [&](UnparkResult result)
                                  {
                                      MK_UNREF(result);
                                      state_ &= (~kEDrainBit);
                                  });
        }

        return;
    }

    // Should we unpark others?
}

bool SharedMutex::tryLockImpl(util::Duration duration)
{
    // TODO(Cheese_S): look into this and see if we can use differet memory order
    util::TimePoint end = util::toTimePoint(duration);

    u32 spinCount = 0;
    u32 current;
    for (;;)
    {
        current = state_.load(std::memory_order_relaxed);

        if (!(current & kELockedBit))
        {
            if (compareExchangeWeak(state_, current, current | kELockedBit))
            {
                // Successfully grabbed the lock. Now, we drain any outstanding readers.
                // No new readers / writers are allowed to enter anymore.
                drainShared();

                // We know there isn't any readers now. We are done.
                return true;
            }
            continue;
        }

        util::TimePoint now = util::Clock::now();
        if (now >= end)
        {
            return false;
        }

        if (spinCount < kMaxSpinCount)
        {
            spinCount++;
            std::this_thread::yield();
            continue;
        }

        // Failed to grab the lock, we wait for whoever grabbed the lock
        if (!(current & kEParkedBit) &&
            !compareExchangeWeak(state_, current, current | kEParkedBit))
        {
            continue;
        }

        ParkResult result = ParkingLot::parkUntil(
            this,
            kEParkedBit,
            [this]
            {
                u32 current = state_.load();
                return ((current & kELockedBit) && (current & kEParkedBit));
            },
            [] {},
            end);

        if (result == ParkResult::eTimeout)
        {
            return false;
        }
    }
}

void SharedMutex::drainShared()
{
    u32 current;
    for (;;)
    {
        current = state_.load(std::memory_order_relaxed);
        if (!(current & kSCountMask))
        {
            return;
        }

        if (!compareExchangeWeak(state_, current, current | kEDrainBit))
        {
            continue;
        }

        MK_IGNORE_RET_VAL ParkingLot::parkUntil(
            this,
            kEDrainBit,
            [this]
            {
                u32 current = state_.load();
                return (current & kSCountMask) && (current & kEDrainBit);
            },
            [] {},
            util::TimePoint::max());
        // state_ &= ~kEDrainBit;
    }
}

bool SharedMutex::tryLockSharedImpl(util::Duration duration)
{
    util::TimePoint now;
    util::TimePoint end = util::toTimePoint(duration);

    u32 current;
    for (;;)
    {
        current = state_.load(std::memory_order_relaxed);
        if (!(current & kEMask))
        {
            if (compareExchangeWeak(state_, current, current + kSIncr))
            {
                return true;
            }
            continue;
        }

        now = util::Clock::now();
        if (now >= end)
        {
            return false;
        }

        if (!compareExchangeWeak(state_, current, current | kSParkedBit))
        {
            continue;
        }

        // Has either waiting writers / existing writers. We sleep.
        // MK_RAW_LOG_INFO("lockShared prepark current: {}", current);
        ParkResult result = ParkingLot::parkUntil(
            this,
            kSParkedBit,
            [this]
            {
                u32 current = state_.load();
                // MK_RAW_LOG_INFO("lockShared park validation state: {}", current);
                return (current & kSParkedBit) && (current & kEMask);
            },
            [] {},
            end);

        if (result == ParkResult::eTimeout)
        {
            return false;
        }
    }
}

} // namespace mk::cc
