#pragma once

#include <thread>
#include <concepts>

#include <core/IType.h>
#include <core/IUniquePtr.h>
#include <core/container/IVector.h>
#include <core/container/ILockFreeObjectPool.h>

#include <core/concurrency/jobsystem/IJob.h>
#include <core/concurrency/jobsystem/IJobInstance.h>
#include <core/concurrency/jobsystem/IJobPriority.h>
#include <core/concurrency/jobsystem/IJobQueue.h>
#include <core/IEnum.h>

#include <concurrentqueue.h>
#include <core/concurrency/ICondition.h>
#include "core/concurrency/jobsystem/IWaitGroup.h"

// Fwd declare
namespace mk::cc
{
class IJobSystem;
}

namespace mk::cc::details
{

using GlobalJobQueue = moodycamel::ConcurrentQueue<details::JobInstance*>;
using LocalJobQueue = JobQueue<1024>;

struct WorkerGroupContext
{
    GlobalJobQueue   highPrioQueue;
    GlobalJobQueue   lowPrioQueue;
    WaitGroup        waitGroup;
    JobPriorityGroup priorityGroup;
};

struct WorkerContext
{
    IJobSystem*                    jobSystem;
    WorkerGroupContext*            groupCtx;
    std::atomic<bool>*             shutdown;
    std::thread                    thread;
    LocalJobQueue                  queue;
    StackVector<LocalJobQueue*, 8> neighborQueues;
};

void runJob(details::JobInstance& instance);

} // namespace mk::cc::details

namespace mk::cc
{

struct JobSystemConfig
{
    u8 numForegroundThreads;
    u8 numBackgorundThreads;
};

struct JobPoolSizeConfig
{
    u32 smallJobPoolSize;
    u32 mediumJobPoolSize;
    u32 largeJobPoolSize;
};

struct JobHandle
{
    static constexpr u32 kInvalidSalt = static_cast<u32>(-1);

    u32 id;
    u32 salt;
};

// fwd declare
template<typename T, typename... ArgsType>
    requires std::derived_from<T, IJob>
JobHandle allocJob(IJobSystem& system, JobPriority priority, ArgsType&&... args);

class IJobSystem
{
public:
    class RunJobPasskey
    {
        MK_NON_MOVABLE_NON_COPYABLE(RunJobPasskey);

    private:
        explicit RunJobPasskey() = default;

        friend void details::runJob(details::JobInstance& instance);
    };

    class AllocJobPasskey
    {
        MK_NON_MOVABLE_NON_COPYABLE(AllocJobPasskey);

    private:
        explicit AllocJobPasskey() = default;

        template<typename T, typename... ArgsType>
            requires std::derived_from<T, IJob>
        friend JobHandle
        allocJob(IJobSystem& system, JobPriority priority, ArgsType&&... args);
    };

    using JobPlacementFn = IJob*(std::byte* bytes, void* args);

    virtual ~IJobSystem() = default;

    virtual JobHandle allocJob(AllocJobPasskey,
                               usize          size,
                               JobPriority    priority,
                               JobPlacementFn fn,
                               void*          args) = 0;

    virtual void scheduleJob(JobHandle handle) = 0;

    virtual void addJobDependency(JobHandle depender, JobHandle dependee) = 0;

    virtual Result waitForJob(JobHandle      handle,
                              bool           executeJobWhileWaiting,
                              util::Duration duration) = 0;

    virtual void runJob(RunJobPasskey, details::JobInstance& instance) = 0;
};

// TODO(Cheese_S): maybe provide speicialization where job count can be zero
// Currently, each job count must be at least 1
template<JobPoolSizeConfig PoolSizeConfig>
class JobSystem: public IJobSystem
{
    static constexpr usize kJobHandleCount = PoolSizeConfig.smallJobPoolSize +
                                             PoolSizeConfig.mediumJobPoolSize +
                                             PoolSizeConfig.largeJobPoolSize;
    static_assert(kJobHandleCount <= UINT32_MAX,
                  "Cannot have more than UINT32_MAX number of jobs");

    static constexpr usize kSmallJobHandleStartIdx = 0;
    static constexpr usize kMediumJobHandleStartIdx =
        kSmallJobHandleStartIdx + PoolSizeConfig.smallJobPoolSize;
    static constexpr usize kLargeJobHandleStartIdx =
        kMediumJobHandleStartIdx + PoolSizeConfig.mediumJobPoolSize;

    struct alignas(64) JobDepdencyNode
    {
        SharedMutex                           mutex;
        StackVector<details::JobInstance*, 8> data;
    };

public:
    MK_NON_MOVABLE_NON_COPYABLE(JobSystem);

    JobSystem(JobSystemConfig config);

    // The user is responsible for ensuring all meaningful work has already finished
    // executing. You can achieve this by having god level jobs.
    ~JobSystem() override;

    // Internal. Call the templated helper function below.
    JobHandle allocJob(AllocJobPasskey,
                       usize          size,
                       JobPriority    priority,
                       JobPlacementFn fn,
                       void*          args) override;

    void scheduleJob(JobHandle handle) override;

    // The depender must be in eAllocated state.
    // The dependee can be in any states, even pointing to a job that has already finished
    void addJobDependency(JobHandle depender, JobHandle dependee) override;

    // Returns eTimeout if job didn't finish within duration
    // Returns eOk if job is already finished / finished within duration.
    // DO NOT CALL executeJobWhileWaiting on the main thread. It will cause undefined
    // behavior.
    Result waitForJob(JobHandle      handle,
                      bool           executeJobWhileWaiting,
                      util::Duration duration) override;

    void runJob(RunJobPasskey, details::JobInstance& instance) override;

private:
    bool isValidHandle(JobHandle handle);

    bool isJobFinished(JobHandle handle);

    void finishJob(details::JobInstance& instance);

    void decrementJobDependencyCount(details::JobInstance& instance);

    void initWorkerContexts(VectorView<details::WorkerContext> ctxs,
                            details::WorkerGroupContext&       groupCtx);

    Condition<SharedMutex>                  condition_;
    StackVector<details::WorkerContext, 32> workerCtxs_;
    details::WorkerGroupContext             groupCtxs_[JobPriorityGroup::eCount];
    std::atomic<bool>                       shutdown_;

    LockFreeObjectPool<SmallJobStorage, PoolSizeConfig.smallJobPoolSize>   smallJobPool_;
    LockFreeObjectPool<MediumJobStorage, PoolSizeConfig.mediumJobPoolSize> mediumJobPool_;
    LockFreeObjectPool<LargeJobStorage, PoolSizeConfig.largeJobPoolSize>   largeJobPool_;

    JobDepdencyNode dependencyGraph_[kJobHandleCount];

    details::JobInstance instances_[kJobHandleCount];
};

template<typename T, typename... ArgsType>
    requires std::derived_from<T, IJob>
JobHandle allocJob(IJobSystem& system, JobPriority priority, ArgsType&&... args)
{
    static_assert(alignof(T) % 64 == 0, "Jobs needs to be at least 64 bytes aligned");
    static_assert(sizeof(T) % 64 == 0, "Job's size needs to be divisible by 64");
    static_assert(sizeof(T) <= 256,
                  "Job's size exceeds the maximum, 256. Consider split up your jobs.");

    auto argsTuple = std::forward_as_tuple(std::forward<ArgsType>(args)...);

    auto placementFn = [](std::byte* bytes, void* ctx) -> IJob*
    {
        auto& tuple = *static_cast<decltype(argsTuple)*>(ctx);
        return std::apply([bytes](auto&&... a) -> IJob*
                          { return ::new (bytes) T(std::forward<decltype(a)>(a)...); },
                          std::move(tuple));
    };

    return system.allocJob(IJobSystem::AllocJobPasskey{},
                           sizeof(T),
                           priority,
                           placementFn,
                           &argsTuple);
}

}; // namespace mk::cc
#define MK_JOBSYSTEM_IMPL
#include <core/concurrency/jobsystem/IJobSystem.inl>
#undef MK_JOBSYSTEM_IMPL
