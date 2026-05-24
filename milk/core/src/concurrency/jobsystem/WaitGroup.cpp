#include <core/concurrency/jobsystem/IWaitGroup.h>
#include <core/concurrency/IParkingLot.h>

namespace mk::cc
{

void WaitGroup::init(u32 numActiveThreads)
{
    numActiveThreads_.store(numActiveThreads, std::memory_order_release);
}

u32 WaitGroup::epoch()
{
    return epoch_.load(std::memory_order_acquire);
}

bool WaitGroup::wait(u32 epoch)
{
    numActiveThreads_.fetch_sub(1, std::memory_order_acq_rel);

    ParkResult result = ParkingLot::parkUntil(
        this,
        kWaitGroupMask,
        [this, epoch]() { return epoch == epoch_.load(); },
        []() {},
        util::toTimePoint(util::kForever));

    // This introduce a race window where the unparker thread can still see 0 before
    // the thread that just woke up decrement the count.
    // We allow this situation to happen.
    numActiveThreads_.fetch_add(1, std::memory_order_acq_rel);

    return result == ParkResult::eUnparked;
}

void WaitGroup::wakeOneIfNoActiveThreads()
{
    epoch_.fetch_add(1, std::memory_order_seq_cst);

    if (numActiveThreads_.load(std::memory_order_acquire))
    {
        return;
    }
    ParkingLot::unparkOne(this,
                          kWaitGroupMask,
                          [](UnparkResult result) { MK_UNREF(result); });
}

void WaitGroup::wakeAll()
{
    epoch_.fetch_add(1, std::memory_order_seq_cst);

    ParkingLot::unparkAll(this, kWaitGroupMask);
}

} // namespace mk::cc
