#pragma once
#include <atomic>
#include <core/IType.h>
#include <core/concurrency/jobsystem/IJobPriority.h>
#include <core/concurrency/jobsystem/IJob.h>

namespace mk::cc::details
{
struct alignas(64) JobInstance
{
    enum class State
    {
        eAllocated, // Job state after being allocated
        eScheduled, // Job state after being pushed to a work queue
        eFinished,  // Job state after being executed
    };

    IJob*              job;
    usize              size;
    JobPriority        priority;
    // Technically, we don't need this id. We can compute it on the fly
    // Remove this if we need more space.
    u32                id;
    std::atomic<State> state;
    std::atomic<u32>   dependencyCount;
    std::atomic<u32>   salt;
};
} // namespace mk::cc::details
