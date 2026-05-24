#pragma once
#include <atomic>

#include <core/IType.h>
#include <core/concurrency/jobsystem/IJob.h>
#include <core/concurrency/jobsystem/IJobInstance.h>
#include <core/IMacro.h>

namespace mk::cc
{
// A SPMC deque. Only the owning thread can call push / pop.
// All threads can call steal.
template<u32 MaxCount>
class JobQueue
{
    struct alignas(64) Element
    {
        static constexpr uptr kFree = 0;
        static constexpr uptr kTaken = 1;

        std::atomic<uptr> value;
    };

public:
    MK_NON_MOVABLE_NON_COPYABLE(JobQueue);

    JobQueue(): head_(0), tail_(0), queue_() {}

    // Can only be called by the owning thread.
    bool tryEnqueue(details::JobInstance* instance);

    // Can only be called by the owning thread.
    // Pop in LIFO order
    bool tryDequeue(details::JobInstance*& outInstance);

    // Can be called by all threads.
    // Steal in FIFO order
    bool trySteal(details::JobInstance*& outInstance);

private:
    alignas(64) u32 head_;
    alignas(64) std::atomic<u32> tail_;
    Element queue_[MaxCount];
};

} // namespace mk::cc
#define MK_JOBQUEUE_IMPL
#include <core/concurrency/jobsystem/IJobQueue.inl>
#undef MK_JOBQUEUE_IMPL
