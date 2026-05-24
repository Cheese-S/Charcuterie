#include <core/IAssert.h>
#include <core/IAtomic.h>
#include <core/IEnum.h>

#ifndef MK_JOBQUEUE_IMPL
    #include <core/concurrency/jobsystem/IJobQueue.h>
#endif

namespace mk::cc
{
template<u32 MaxCount>
bool JobQueue<MaxCount>::tryEnqueue(details::JobInstance* instance)
{
    MK_ASSERT(instance);
    u32  index = head_ % MaxCount;
    uptr bits = queue_[index].value.load(std::memory_order_acquire);

    if (bits != Element::kFree)
    {
        return false;
    }

    queue_[index].value.store(std::bit_cast<uptr>(instance), std::memory_order_release);
    head_++;
    return true;
}

template<u32 MaxCount>
bool JobQueue<MaxCount>::tryDequeue(details::JobInstance*& outInstance)
{
    u32  index = (head_ - 1) % MaxCount;
    uptr bits = queue_[index].value.load(std::memory_order_acquire);

    if (bits == Element::kFree || bits == Element::kTaken)
    {
        return false;
    }

    if (queue_[index].value.compare_exchange_strong(bits,
                                                    Element::kFree,
                                                    std::memory_order_acq_rel))
    {
        head_--;
        outInstance = std::bit_cast<details::JobInstance*>(bits);
        return true;
    }

    return false;
}

template<u32 MaxCount>
bool JobQueue<MaxCount>::trySteal(details::JobInstance*& outInstance)
{
    u32  tail;
    u32  index;
    uptr bits;

    while (true)
    {
        tail = tail_.load(std::memory_order_acquire);
        index = tail % MaxCount;

        bits = std::bit_cast<uptr>(queue_[index].value.load(std::memory_order_acquire));

        if (bits == Element::kFree)
        {
            if (tail == tail_.load(std::memory_order_acquire))
            {
                // This means that the queue is indeed empty
                // No other threads popped / stole something concurretnly
                return false;
            }
        }
        else if (bits != Element::kTaken &&
                 compareExchangeWeak(queue_[index].value, bits, Element::kTaken))
        {
            // This check is needed to prevent ABA problems.
            if (tail == tail_.load(std::memory_order_acquire))
            {
                tail_.fetch_add(1, std::memory_order_acq_rel);
                queue_[index].value.store(Element::kFree, std::memory_order_release);
                outInstance = std::bit_cast<details::JobInstance*>(bits);
                return true;
            }
            queue_[index].value.store(bits, std::memory_order_release);
        }
    }
}

} // namespace mk::cc
