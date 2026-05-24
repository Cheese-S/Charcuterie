#pragma once

#include <core/IMacro.h>
#include <core/IType.h>
#include <atomic>

namespace mk::cc
{

class WaitGroup
{
    static constexpr u32 kWaitGroupMask = 0xFFFFFFFF;

public:
    MK_NON_MOVABLE_NON_COPYABLE(WaitGroup);

    WaitGroup(): epoch_(0), numActiveThreads_(0) {};

    // Should only be called once.
    void init(u32 numActiveThreads);

    bool wait(u32 epoch);
    u32  epoch();

    void wakeOne();
    void wakeOneIfNoActiveThreads();

    // Should only be used during shutdown
    void wakeAll();

private:
    std::atomic<u32> epoch_;
    std::atomic<u32> numActiveThreads_;
};

}; // namespace mk::cc
