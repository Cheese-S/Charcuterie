#include <core/concurrency/jobsystem/IJobSystem.h>
#include <core/concurrency/IScopedLock.h>
#include "core/concurrency/IPause.h"

namespace mk::cc::details
{

MK_DEFINE_DEFAULT_LOG_CATEGORY(JobSystem);

namespace
{

thread_local WorkerContext* tlsCtx;

bool tryDequeueJob(WorkerContext& ctx, JobInstance*& outInstance)
{
    // TODO(Cheese_S): implement fairness using a counter
    if (ctx.groupCtx->highPrioQueue.try_dequeue(outInstance))
    {
        return true;
    }

    if (ctx.groupCtx->lowPrioQueue.try_dequeue(outInstance))
    {
        return true;
    }

    if (ctx.queue.tryDequeue(outInstance))
    {
        return true;
    }

    for (auto& neighborQueue : ctx.neighborQueues)
    {
        if (neighborQueue->trySteal(outInstance))
        {
            return true;
        }
    }

    return false;
}

} // namespace

void runJob(details::JobInstance& instance)
{
    MK_ASSERT(tlsCtx);
    tlsCtx->jobSystem->runJob(IJobSystem::RunJobPasskey{}, instance);
}

void jobThreadFn(WorkerContext& ctx)
{
    static constexpr u8 kMaxSpinCount = 40;
    static constexpr u8 kMaxYieldCount = 2;

    u8           spinCount = 0;
    u8           yieldCount = 0;
    u32          epoch = 0;
    JobInstance* instance;

    tlsCtx = &ctx;

    while (true)
    {
        epoch = ctx.groupCtx->waitGroup.epoch();
        if (tryDequeueJob(ctx, instance))
        {
            runJob(*instance);
            continue;
        }

        if (spinCount < kMaxSpinCount)
        {
            spinCount++;
            pause();
            continue;
        }

        if (yieldCount < kMaxYieldCount)
        {
            yieldCount++;
            std::this_thread::yield();
        }

        if (ctx.shutdown->load(std::memory_order_relaxed))
        {
            return;
        }

        bool didSleep = ctx.groupCtx->waitGroup.wait(epoch);

        if (didSleep)
        {
            spinCount = 0;
            yieldCount = 0;
        }
    }
}

void tryRunOneJob()
{
    static constexpr u8 kMaxSpinCount = 40;
    static constexpr u8 kMaxYieldCount = 2;

    MK_ASSERT(tlsCtx);

    details::JobInstance* instance;

    u8 spinCount = 0;
    u8 yieldCount = 0;

    while (true)
    {
        if (tryDequeueJob(*tlsCtx, instance))
        {
            runJob(*instance);
            return;
        }

        if (spinCount < kMaxSpinCount)
        {
            pause();
            spinCount++;
            continue;
        }

        if (yieldCount < kMaxYieldCount)
        {
            std::this_thread::yield();
            yieldCount++;
            continue;
        }

        return;
    }
}

WorkerContext* getTlsWorkerContext()
{
    return tlsCtx;
}
} // namespace mk::cc::details
