#pragma once

#ifndef MK_JOBSYSTEM_IMPL
    #include <core/concurrency/jobsystem/IJobSystem.h>
#endif

#include <core/IAppContext.h>
#include <core/log/ILog.h>
#include <core/log/ILogCategory.h>

namespace mk::cc::details
{

WorkerContext* getTlsWorkerContext();
void           jobThreadFn(WorkerContext& ctx);
void           tryRunOneJob();

inline JobPriorityGroup toPriorityGroup(JobPriority priority)
{
    switch (priority)
    {
    case JobPriority::eHigh:
    case JobPriority::eLow:
        return JobPriorityGroup::eForeground;
    case JobPriority::eBackgroundHigh:
    case JobPriority::eBackgroundLow:
        return JobPriorityGroup::eBackground;
    default:
        MK_ASSERT_UNREACHABLE();
        return JobPriorityGroup::eForeground;
        break;
    }
}

} // namespace mk::cc::details

namespace mk::cc
{

template<JobPoolSizeConfig PoolSizeConfig>
JobSystem<PoolSizeConfig>::JobSystem(JobSystemConfig config):
    shutdown_(false), smallJobPool_(1), mediumJobPool_(1), largeJobPool_(1)
{
    // TODO(Cheese_S): Should we init worker ctx for main thread so it can execute job
    // while waiting?
    u8 numTotalThreads = config.numBackgorundThreads + config.numForegroundThreads;
    if (numTotalThreads >= std::thread::hardware_concurrency())
    {
        MK_CAT_LOG_WARN(
            JobSystem,
            "Allocating more logic threads than the hardware supports. This can cause "
            "performance drop. Allocating: %lu, hardware supports: %lu",
            numTotalThreads,
            std::thread::hardware_concurrency());
    }

    // init job handles
    {
        for (usize i = 0; i < kJobHandleCount; i++)
        {
            instances_[i].job = nullptr;
            instances_[i].dependencyCount = 0;
            instances_[i].salt.store(JobHandle::kInvalidSalt, std::memory_order_release);
            instances_[i].id = i;
        }
    }

    // init group contexts
    {
        groupCtxs_[JobPriorityGroup::eBackground].waitGroup.init(
            config.numBackgorundThreads);

        groupCtxs_[JobPriorityGroup::eForeground].waitGroup.init(
            config.numForegroundThreads);
    }

    // init worker contexts
    {
        workerCtxs_.resize(numTotalThreads);

        initWorkerContexts(
            { workerCtxs_.begin(), workerCtxs_.begin() + config.numForegroundThreads },
            groupCtxs_[JobPriorityGroup::eForeground]);

        initWorkerContexts(
            { workerCtxs_.begin() + config.numForegroundThreads, workerCtxs_.end() },
            groupCtxs_[JobPriorityGroup::eBackground]);
    }
}

template<JobPoolSizeConfig PoolSizeConfig>
void JobSystem<PoolSizeConfig>::initWorkerContexts(
    VectorView<details::WorkerContext> ctxs,
    details::WorkerGroupContext&       groupCtx)
{
    for (auto& ctx : ctxs)
    {
        ctx.jobSystem = this;
        ctx.groupCtx = &groupCtx;
        ctx.shutdown = &shutdown_;
        for (auto& neighborCtx : ctxs)
        {
            if (&ctx == &neighborCtx)
            {
                continue;
            }
            ctx.neighborQueues.push(&neighborCtx.queue);
        }

        ctx.thread = std::thread([&ctx]() { jobThreadFn(ctx); });
    }
}

template<JobPoolSizeConfig PoolSizeConfig>
JobSystem<PoolSizeConfig>::~JobSystem()
{
    shutdown_.store(true, std::memory_order_release);

    {
        groupCtxs_[JobPriorityGroup::eForeground].waitGroup.wakeAll();
        groupCtxs_[JobPriorityGroup::eBackground].waitGroup.wakeAll();
    }

    for (auto& workerCtx : workerCtxs_)
    {
        workerCtx.thread.join();
    }
}

template<JobPoolSizeConfig PoolSizeConfig>
JobHandle JobSystem<PoolSizeConfig>::allocJob(AllocJobPasskey,
                                              usize          size,
                                              JobPriority    priority,
                                              JobPlacementFn fn,
                                              void*          args)
{
    std::byte* bytes = nullptr;
    u32        index;
    if (size == sizeof(SmallJobStorage))
    {
        SmallJobStorage* storage = smallJobPool_.tryAcquire();
        MK_ASSERTF(storage,
                   "[Job System]: overallocated small job storage. Consider increasing "
                   "the limit.");
        index = kSmallJobHandleStartIdx + smallJobPool_.index(storage);
        bytes = storage->bytes;
    }
    else if (size == sizeof(MediumJobStorage))
    {
        MediumJobStorage* storage = mediumJobPool_.tryAcquire();
        MK_ASSERTF(storage,
                   "[Job System]: overallocated medium job storage. Consider increasing "
                   "the limit.");
        index = kMediumJobHandleStartIdx + mediumJobPool_.index(storage);
        bytes = storage->bytes;
    }
    else if (size == sizeof(LargeJobStorage))
    {
        LargeJobStorage* storage = largeJobPool_.tryAcquire();
        MK_ASSERTF(storage,
                   "[Job System]: overallocated large job storage. Consider increasing "
                   "the limit.");
        index = kLargeJobHandleStartIdx + largeJobPool_.index(storage);
        bytes = storage->bytes;
    }
    else
    {
        MK_ASSERT_UNREACHABLE();
    }

    if (!bytes)
    {
        return JobHandle{
            .id = 0,
            .salt = JobHandle::kInvalidSalt,
        };
    }

    IJob* job = fn(bytes, args);

    // Each job instance has an implicit global dependency.
    // This will be removed in scheduleJob.
    u32                   salt;
    details::JobInstance& instance = instances_[index];
    instance.job = job;
    instance.priority = priority;
    instance.dependencyCount.store(1, std::memory_order_release);
    instance.size = size;
    instance.state.store(details::JobInstance::State::eAllocated,
                         std::memory_order_release);
    salt = instance.salt.load(std::memory_order_acquire);
    if (salt == JobHandle::kInvalidSalt) [[unlikely]]
    {
        salt = 0;
        instance.salt.store(0, std::memory_order_release);
    }

    MK_ASSERT(index == instance.id);

    return JobHandle{
        .id = index,
        .salt = salt,
    };
}

template<JobPoolSizeConfig PoolSizeConfig>
void JobSystem<PoolSizeConfig>::scheduleJob(JobHandle handle)
{
    MK_ASSERTF(isValidHandle(handle),
               "[Job System]: only a valid job handle can be scheduled.");
    [[maybe_unused]] details::JobInstance::State state;
    state = instances_[handle.id].state.load(std::memory_order_acquire);
    MK_ASSERTF(state == details::JobInstance::State::eAllocated,
               "[Job System]: only a job in eAllocated state can be scheduled. Current "
               "state: {}",
               enum_utils::toUnderlying(state));

    details::JobInstance& instance = instances_[handle.id];
    decrementJobDependencyCount(instance);
}

template<JobPoolSizeConfig PoolSizeConfig>
Result JobSystem<PoolSizeConfig>::waitForJob(JobHandle      handle,
                                             bool           executeJobWhileWaiting,
                                             util::Duration duration)
{
    MK_ASSERTF(isValidHandle(handle),
               "[Job System]: only can wait for a valid job handle.");

    static util::Duration kPollPeriod = std::chrono::milliseconds(1);

    util::TimePoint now;
    util::TimePoint end = util::toTimePoint(duration);

    while (!isJobFinished(handle))
    {
        now = util::Clock::now();
        if (now >= end)
        {
            return Result::eTimeout;
        }

        if (executeJobWhileWaiting)
        {
            details::tryRunOneJob();
        }
        else
        {
            std::this_thread::sleep_for(kPollPeriod);
        }
    }

    return Result::eOk;
}

template<JobPoolSizeConfig PoolSizeConfig>
void JobSystem<PoolSizeConfig>::addJobDependency(JobHandle dependerHandle,
                                                 JobHandle dependeeHandle)
{
    MK_ASSERTF(isValidHandle(dependerHandle) &&
                   instances_[dependerHandle.id].state.load(std::memory_order_relaxed) ==
                       details::JobInstance::State::eAllocated,
               "[Job System]: depender must be a valid job handle in eAllocated state");
    MK_ASSERT(dependeeHandle.id < kJobHandleCount);

    details::JobInstance& dependeeInstance = instances_[dependeeHandle.id];
    if (dependeeInstance.salt.load(std::memory_order_acquire) != dependeeHandle.salt)
    {
        return;
    }

    details::JobInstance& dependerInstance = instances_[dependerHandle.id];
    JobDepdencyNode&      node = dependencyGraph_[dependeeInstance.id];
    {
        ScopedLock<SharedMutex> lock(node.mutex);
        if (dependeeInstance.salt.load(std::memory_order_acquire) ==
                dependeeHandle.salt &&
            dependeeInstance.state.load(std::memory_order_acquire) <
                details::JobInstance::State::eFinished)
        {
            node.data.push(&dependerInstance);
            dependerInstance.dependencyCount.fetch_add(1, std::memory_order_acq_rel);
        }
    }
}

template<JobPoolSizeConfig PoolSizeConfig>
void JobSystem<PoolSizeConfig>::runJob(RunJobPasskey, details::JobInstance& instance)
{
    instance.job->run();
    finishJob(instance);
}

template<JobPoolSizeConfig PoolSizeConfig>
void JobSystem<PoolSizeConfig>::finishJob(details::JobInstance& instance)
{
    JobDepdencyNode& node = dependencyGraph_[instance.id];

    {
        ScopedLock<SharedMutex> lock(node.mutex);
        instance.state.store(details::JobInstance::State::eFinished,
                             std::memory_order_release);
        for (auto& depender : node.data)
        {
            decrementJobDependencyCount(*depender);
        }
        node.data.clear();
    }

    u32 prevSalt = instance.salt.fetch_add(1, std::memory_order_acq_rel);

    if (prevSalt == JobHandle::kInvalidSalt - 1)
    {
        instance.salt.store(0, std::memory_order_release);
    }

    if (instance.size <= sizeof(SmallJobStorage))
    {
        SmallJobStorage* storage = reinterpret_cast<SmallJobStorage*>(instance.job);
        smallJobPool_.release(storage);
    }
    else if (instance.size <= sizeof(MediumJobStorage))
    {
        MediumJobStorage* storage = reinterpret_cast<MediumJobStorage*>(instance.job);
        mediumJobPool_.release(storage);
    }
    else if (instance.size <= sizeof(LargeJobStorage))
    {
        LargeJobStorage* storage = reinterpret_cast<LargeJobStorage*>(instance.job);
        largeJobPool_.release(storage);
    }
    else
    {
        MK_ASSERT_UNREACHABLE();
    }
}

template<JobPoolSizeConfig PoolSizeConfig>
void JobSystem<PoolSizeConfig>::decrementJobDependencyCount(
    details::JobInstance& instance)
{
    u32 prevCount = instance.dependencyCount.fetch_sub(1, std::memory_order_acq_rel);
    // MK_RAW_LOG_INFO("decrementing id: {}, prev: {}", instance.id, prevCount);
    MK_ASSERTF(prevCount,
               "[Job System]: Cannot remove dependency on job instance {} that has no "
               "dependency left.",
               instance.id);
    if (prevCount != 1)
    {
        return;
    }

    details::WorkerContext* ctx = details::getTlsWorkerContext();

    details::GlobalJobQueue* globalQueue = nullptr;
    details::LocalJobQueue*  localQueue = nullptr;

    if (ctx &&
        ctx->groupCtx->priorityGroup == details::toPriorityGroup(instance.priority))
    {
        localQueue = &ctx->queue;
    }

    details::WorkerGroupContext* groupCtx;
    switch (instance.priority)
    {
    case JobPriority::eHigh:
        groupCtx = &groupCtxs_[JobPriorityGroup::eForeground];
        globalQueue = &groupCtxs_[JobPriorityGroup::eForeground].highPrioQueue;
        break;
    case JobPriority::eLow:
        groupCtx = &groupCtxs_[JobPriorityGroup::eForeground];
        globalQueue = &groupCtxs_[JobPriorityGroup::eForeground].lowPrioQueue;
        break;
    case JobPriority::eBackgroundHigh:
        groupCtx = &groupCtxs_[JobPriorityGroup::eBackground];
        globalQueue = &groupCtxs_[JobPriorityGroup::eBackground].highPrioQueue;
        break;
    case JobPriority::eBackgroundLow:
        groupCtx = &groupCtxs_[JobPriorityGroup::eBackground];
        globalQueue = &groupCtxs_[JobPriorityGroup::eBackground].lowPrioQueue;
        break;
    default:
        MK_ASSERT_UNREACHABLE();
        groupCtx = &groupCtxs_[JobPriorityGroup::eForeground];
        globalQueue = &groupCtxs_[JobPriorityGroup::eForeground].highPrioQueue;
        break;
    }

    details::JobInstance::State state = details::JobInstance::State::eAllocated;
    bool                        success =
        instance.state.compare_exchange_strong(state,
                                               details::JobInstance::State::eScheduled);
    MK_ASSERT(success);

    if (localQueue && localQueue->tryEnqueue(&instance))
    {
        return;
    }

    globalQueue->enqueue(&instance);
    groupCtx->waitGroup.wakeOneIfNoActiveThreads();
}

template<JobPoolSizeConfig PoolSizeConfig>
bool JobSystem<PoolSizeConfig>::isValidHandle(JobHandle handle)
{
    MK_ASSERT(handle.id <= kJobHandleCount);

    return handle.salt != JobHandle::kInvalidSalt &&
           handle.salt == instances_[handle.id].salt.load(std::memory_order_relaxed);
}

template<JobPoolSizeConfig PoolSizeConfig>
bool JobSystem<PoolSizeConfig>::isJobFinished(JobHandle handle)
{
    MK_ASSERT(handle.id <= kJobHandleCount);

    details::JobInstance& instance = instances_[handle.id];
    return handle.salt != instance.salt.load(std::memory_order_relaxed) ||
           instance.state.load(std::memory_order_relaxed) ==
               details::JobInstance::State::eFinished;
}

} // namespace mk::cc
