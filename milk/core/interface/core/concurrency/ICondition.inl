
#ifndef MK_CONDITION_IMPL
    #include <core/concurrency/ICondition.h>
#endif

namespace mk::cc
{

namespace
{
[[maybe_unused]] Result toResult(ParkResult result)
{
    switch (result)
    {
    case ParkResult::eUnparked:
        return Result::eOk;
    case ParkResult::eTimeout:
        return Result::eTimeout;
    default:
        MK_ASSERT_UNREACHABLE();
        return Result::eOk;
    }
}
} // namespace

template<typename T>
    requires IsLockable<T>
Result Condition<T>::wait(T& lock, util::Duration duration)
{
    util::TimePoint time = util::toTimePoint(duration);

    ParkResult result = ParkingLot::parkUntil(
        this,
        kMask,
        [this]()
        {
            // We need to set hasWaiters here to avoid lost wake ups.
            // ParkingLot's bucket lock ensures that enqueueing + setting hasWaiters
            // is atomic.
            hasWaiters_.store(true, std::memory_order_release);
            return true;
        },
        [&lock]() { lock.unlock(); },
        time);

    lock.lock();

    return toResult(result);
}

template<typename T>
    requires IsLockable<T>
void Condition<T>::notifyOne()
{
    if (!hasWaiters_.load(std::memory_order_acquire))
    {
        return;
    }

    ParkingLot::unparkOne(this,
                          kMask,
                          [this](UnparkResult result)
                          {
                              if (result == UnparkResult::eNoneLeft)
                              {
                                  hasWaiters_.store(false, std::memory_order_release);
                              }
                          });
}

template<typename T>
    requires IsLockable<T>
void Condition<T>::notifyAll()
{
    if (!hasWaiters_.load(std::memory_order_acquire))
    {
        return;
    }

    hasWaiters_.store(false, std::memory_order_release);
    ParkingLot::unparkAll(this, kMask);
}

} // namespace mk::cc
