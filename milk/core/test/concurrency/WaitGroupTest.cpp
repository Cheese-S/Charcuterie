#include <gtest/gtest.h>
#include <thread>
#include <atomic>
#include <chrono>
#include <vector>

#include <core/concurrency/jobsystem/IWaitGroup.h>
#include "core/log/IRawLog.h"

// NOTE: These tests are written assuming the intended semantics
// (decrement by 1 before parking, increment by 1 after waking).

namespace mk::cc
{

// ---------------------------------------------------------------------------
// Constants
// ---------------------------------------------------------------------------

// static constexpr auto kShortWait = std::chrono::milliseconds(50);
// static constexpr auto kLongWait = std::chrono::milliseconds(500);
// static constexpr u32  kStressWaiterCount = 8;
// static constexpr u32  kStressThreadCount = 16;
// static constexpr u32  kEpochCycleCount = 4;

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

static bool spinWaitFor(const std::atomic<bool>& flag, std::chrono::milliseconds timeout)
{
    const auto deadline = std::chrono::steady_clock::now() + timeout;
    while (!flag.load(std::memory_order_acquire))
    {
        if (std::chrono::steady_clock::now() >= deadline)
        {
            return false;
        }
        std::this_thread::yield();
    }
    return true;
}

// static void spinWaitForCount(const std::atomic<u32>&   counter,
//                              u32                       target,
//                              std::chrono::milliseconds timeout = kLongWait)
// {
//     const auto deadline = std::chrono::steady_clock::now() + timeout;
//     while (counter.load(std::memory_order_acquire) < target)
//     {
//         ASSERT_LT(std::chrono::steady_clock::now(), deadline)
//             << "Timed out waiting for counter to reach " << target;
//         std::this_thread::yield();
//     }
// }
//
// ---------------------------------------------------------------------------
// Fixture
// ---------------------------------------------------------------------------

class WaitGroupTest: public ::testing::Test
{
protected:
    void TearDown() override
    {
        joinAll();
    }

    void track(std::thread t)
    {
        threads_.push_back(std::move(t));
    }

    // Call at the end of any test whose threads hold references to test-local
    // variables. Joining here (while locals are still in scope) is safe;
    // the implicit join in TearDown would race against their destruction.
    void joinAll()
    {
        for (auto& t : threads_)
        {
            if (t.joinable())
            {
                t.join();
            }
        }
        threads_.clear();
    }

private:
    std::vector<std::thread> threads_;
};

// ---------------------------------------------------------------------------
// Construction
// ---------------------------------------------------------------------------
//
// TEST_F(WaitGroupTest, ConstructsWithOneActiveThread)
// {
//     EXPECT_NO_FATAL_FAILURE({
//         WaitGroup wg;
//         wg.init(1);
//     });
// }
//
// TEST_F(WaitGroupTest, ConstructsWithManyActiveThreads)
// {
//     EXPECT_NO_FATAL_FAILURE({
//         WaitGroup wg;
//         wg.init(8);
//     });
// }
//
// // ---------------------------------------------------------------------------
// // wakeOneIfNoActiveThreads — no-op when count > 0 after decrement
// // ---------------------------------------------------------------------------
//
// TEST_F(WaitGroupTest, DoesNotWakeWhenOneActiveThreadRemains)
// {
//     WaitGroup wg;
//     wg.init(2);
//     std::atomic<bool> parked{ false };
//     std::atomic<bool> done{ false };
//
//     track(std::thread(
//         [&]()
//         {
//             parked.store(true, std::memory_order_release);
//             wg.wait();
//             done.store(true, std::memory_order_release);
//         }));
//
//     ASSERT_TRUE(spinWaitFor(parked, kLongWait));
//     std::this_thread::sleep_for(kShortWait); // let the thread fully park
//
//     wg.wakeOneIfNoActiveThreads();
//
//     std::this_thread::sleep_for(kShortWait);
//     EXPECT_FALSE(done.load());
//
//     wg.wakeAll();
//     joinAll();
// }
//
// TEST_F(WaitGroupTest, DoesNotWakeWhenManyActiveThreadsRemain)
// {
//     static constexpr u32 kTotalThreads = 4;
//     static constexpr u32 kParkedThreads = 2; // leaves 2 still "active"
//
//     WaitGroup wg;
//     wg.init(kTotalThreads);
//     std::atomic<u32> parkedCount{ 0 };
//     std::atomic<u32> wokenCount{ 0 };
//
//     for (u32 i = 0; i < kParkedThreads; ++i)
//     {
//         track(std::thread(
//             [&]()
//             {
//                 parkedCount.fetch_add(1, std::memory_order_acq_rel);
//                 wg.wait();
//                 wokenCount.fetch_add(1, std::memory_order_acq_rel);
//             }));
//     }
//
//     spinWaitForCount(parkedCount, kParkedThreads);
//     std::this_thread::sleep_for(kShortWait);
//
//     wg.wakeOneIfNoActiveThreads();
//
//     std::this_thread::sleep_for(kShortWait);
//     EXPECT_EQ(wokenCount.load(), 0u);
//
//     wg.wakeAll();
//     joinAll();
// }
//
// // ---------------------------------------------------------------------------
// // wakeOneIfNoActiveThreads — wakes exactly one when count reaches 0
// // ---------------------------------------------------------------------------
//
// TEST_F(WaitGroupTest, WakesOneWaiterWhenAllThreadsAreWaiting)
// {
//     WaitGroup wg;
//     wg.init(1);
//     std::atomic<bool> parked{ false };
//     std::atomic<bool> done{ false };
//     bool              waitResult = false;
//
//     track(std::thread(
//         [&]()
//         {
//             parked.store(true, std::memory_order_release);
//             waitResult = wg.wait();
//             done.store(true, std::memory_order_release);
//         }));
//
//     ASSERT_TRUE(spinWaitFor(parked, kLongWait));
//     std::this_thread::sleep_for(kShortWait);
//
//     wg.wakeOneIfNoActiveThreads();
//
//     EXPECT_TRUE(spinWaitFor(done, kLongWait));
//     EXPECT_TRUE(waitResult);
//
//     joinAll();
// }
//
// TEST_F(WaitGroupTest, WakesExactlyOneOfManyWaitersWhenCountReachesZero)
// {
//     static constexpr u32 kWaiterCount = 3;
//
//     WaitGroup wg;
//     wg.init(kWaiterCount);
//     std::atomic<u32> parkedCount{ 0 };
//     std::atomic<u32> wokenCount{ 0 };
//
//     for (u32 i = 0; i < kWaiterCount; ++i)
//     {
//         track(std::thread(
//             [&]()
//             {
//                 parkedCount.fetch_add(1, std::memory_order_acq_rel);
//                 wg.wait();
//                 wokenCount.fetch_add(1, std::memory_order_acq_rel);
//             }));
//     }
//
//     spinWaitForCount(parkedCount, kWaiterCount);
//     std::this_thread::sleep_for(kShortWait);
//
//     wg.wakeOneIfNoActiveThreads();
//
//     std::this_thread::sleep_for(kShortWait);
//     EXPECT_EQ(wokenCount.load(), 1u);
//
//     wg.wakeAll();
//     joinAll();
// }
//
// // ---------------------------------------------------------------------------
// // wait() return value
// // ---------------------------------------------------------------------------
//
// TEST_F(WaitGroupTest, WaitReturnsTrueWhenWokenByWakeOne)
// {
//     WaitGroup wg;
//     wg.init(1);
//     std::atomic<bool> parked{ false };
//     std::atomic<bool> done{ false };
//     bool              waitResult = false;
//
//     track(std::thread(
//         [&]()
//         {
//             parked.store(true, std::memory_order_release);
//             waitResult = wg.wait();
//             done.store(true, std::memory_order_release);
//         }));
//
//     ASSERT_TRUE(spinWaitFor(parked, kLongWait));
//     std::this_thread::sleep_for(kShortWait);
//
//     wg.wakeOneIfNoActiveThreads();
//
//     ASSERT_TRUE(spinWaitFor(done, kLongWait));
//     EXPECT_TRUE(waitResult);
//
//     joinAll();
// }
//
// // ---------------------------------------------------------------------------
// // wakeAll
// // ---------------------------------------------------------------------------
//
// TEST_F(WaitGroupTest, WakeAllReleasesAllWaiters)
// {
//     static constexpr u32 kWaiterCount = 4;
//
//     WaitGroup wg;
//     wg.init(kWaiterCount);
//     std::atomic<u32> parkedCount{ 0 };
//     std::atomic<u32> wokenCount{ 0 };
//
//     for (u32 i = 0; i < kWaiterCount; ++i)
//     {
//         track(std::thread(
//             [&]()
//             {
//                 parkedCount.fetch_add(1, std::memory_order_acq_rel);
//                 wg.wait();
//                 wokenCount.fetch_add(1, std::memory_order_acq_rel);
//             }));
//     }
//
//     spinWaitForCount(parkedCount, kWaiterCount);
//     std::this_thread::sleep_for(kShortWait);
//
//     wg.wakeAll();
//
//     spinWaitForCount(wokenCount, kWaiterCount);
//     EXPECT_EQ(wokenCount.load(), kWaiterCount);
//
//     joinAll();
// }
//
// TEST_F(WaitGroupTest, WakeAllWithNoWaitersDoesNotCrash)
// {
//     WaitGroup wg;
//     wg.init(1);
//     EXPECT_NO_FATAL_FAILURE({ wg.wakeAll(); });
// }
//
// TEST_F(WaitGroupTest, WakeAllIgnoresActiveThreadCount)
// {
//     static constexpr u32 kExpectedThreads = 4;
//     static constexpr u32 kActualWaiters = 2;
//
//     WaitGroup wg;
//     wg.init(kExpectedThreads);
//     std::atomic<u32> parkedCount{ 0 };
//     std::atomic<u32> wokenCount{ 0 };
//
//     for (u32 i = 0; i < kActualWaiters; ++i)
//     {
//         track(std::thread(
//             [&]()
//             {
//                 parkedCount.fetch_add(1, std::memory_order_acq_rel);
//                 wg.wait();
//                 wokenCount.fetch_add(1, std::memory_order_acq_rel);
//             }));
//     }
//
//     spinWaitForCount(parkedCount, kActualWaiters);
//     std::this_thread::sleep_for(kShortWait);
//
//     wg.wakeAll();
//
//     spinWaitForCount(wokenCount, kActualWaiters);
//     EXPECT_EQ(wokenCount.load(), kActualWaiters);
//
//     joinAll();
// }
//
// // ---------------------------------------------------------------------------
// // Epoch / re-use
// // ---------------------------------------------------------------------------
//
// TEST_F(WaitGroupTest, WaitGroupIsReusableAcrossMultipleCycles)
// {
//     WaitGroup wg;
//     wg.init(1);
//
//     for (u32 cycle = 0; cycle < kEpochCycleCount; ++cycle)
//     {
//         std::atomic<bool> parked{ false };
//         std::atomic<bool> done{ false };
//         bool              result = false;
//
//         std::thread t(
//             [&]()
//             {
//                 parked.store(true, std::memory_order_release);
//                 result = wg.wait();
//                 done.store(true, std::memory_order_release);
//             });
//
//         ASSERT_TRUE(spinWaitFor(parked, kLongWait));
//         std::this_thread::sleep_for(kShortWait);
//         EXPECT_FALSE(done.load())
//             << "Cycle " << cycle << ": wait() returned without a wake";
//
//         wg.wakeOneIfNoActiveThreads();
//         EXPECT_TRUE(spinWaitFor(done, kLongWait))
//             << "Cycle " << cycle << ": wait() did not return";
//
//         t.join();
//         EXPECT_TRUE(result) << "Cycle " << cycle;
//     }
// }
//
// TEST_F(WaitGroupTest, SecondWaitBlocksAfterFirstCycleCompletes)
// {
//     WaitGroup wg;
//     wg.init(1);
//
//     // First cycle
//     {
//         std::atomic<bool> parked{ false };
//         std::atomic<bool> done{ false };
//
//         std::thread t(
//             [&]()
//             {
//                 parked.store(true, std::memory_order_release);
//                 wg.wait();
//                 done.store(true, std::memory_order_release);
//             });
//
//         ASSERT_TRUE(spinWaitFor(parked, kLongWait));
//         std::this_thread::sleep_for(kShortWait);
//         wg.wakeOneIfNoActiveThreads();
//         ASSERT_TRUE(spinWaitFor(done, kLongWait));
//         t.join();
//     }
//
//     // Second cycle — must block, not fall through on the new epoch
//     {
//         std::atomic<bool> parked{ false };
//         std::atomic<bool> done{ false };
//
//         std::thread t(
//             [&]()
//             {
//                 parked.store(true, std::memory_order_release);
//                 wg.wait();
//                 done.store(true, std::memory_order_release);
//             });
//
//         ASSERT_TRUE(spinWaitFor(parked, kLongWait));
//         std::this_thread::sleep_for(kShortWait);
//         EXPECT_FALSE(done.load()) << "wait() fell through immediately on second use";
//
//         wg.wakeAll();
//         t.join();
//     }
// }
//
// // ---------------------------------------------------------------------------
// // Stress
// // ---------------------------------------------------------------------------
//
// TEST_F(WaitGroupTest, StressWakeOneWakesFirstThenWakeAllDrainsRest)
// {
//     WaitGroup wg;
//     wg.init(kStressWaiterCount);
//     std::atomic<u32> parkedCount{ 0 };
//     std::atomic<u32> wokenCount{ 0 };
//
//     for (u32 i = 0; i < kStressWaiterCount; ++i)
//     {
//         track(std::thread(
//             [&]()
//             {
//                 parkedCount.fetch_add(1, std::memory_order_acq_rel);
//                 wg.wait();
//                 wokenCount.fetch_add(1, std::memory_order_acq_rel);
//             }));
//     }
//
//     spinWaitForCount(parkedCount, kStressWaiterCount);
//     std::this_thread::sleep_for(kShortWait);
//
//     // wakeOneIfNoActiveThreads can only fire once here: the first woken thread
//     // increments numActiveThreads_ back to 1 before returning, so any subsequent
//     // wakeOne call sees a non-zero count and skips unparkOne. Use wakeAll to
//     // drain the remaining threads.
//     wg.wakeOneIfNoActiveThreads();
//     spinWaitForCount(wokenCount, 1u);
//     EXPECT_EQ(wokenCount.load(), 1u);
//
//     wg.wakeAll();
//     spinWaitForCount(wokenCount, kStressWaiterCount);
//     EXPECT_EQ(wokenCount.load(), kStressWaiterCount);
//     joinAll();
// }
//
// TEST_F(WaitGroupTest, StressConcurrentWakeOneCallsDoNotCorruptState)
// {
//     WaitGroup wg;
//     wg.init(1);
//     std::atomic<bool> parked{ false };
//     std::atomic<bool> done{ false };
//     std::atomic<u32>  wokenCount{ 0 };
//
//     track(std::thread(
//         [&]()
//         {
//             parked.store(true, std::memory_order_release);
//             wg.wait();
//             wokenCount.fetch_add(1, std::memory_order_acq_rel);
//             done.store(true, std::memory_order_release);
//         }));
//
//     ASSERT_TRUE(spinWaitFor(parked, kLongWait));
//     std::this_thread::sleep_for(kShortWait);
//
//     std::vector<std::thread> wakers;
//     wakers.reserve(kStressThreadCount);
//     for (u32 i = 0; i < kStressThreadCount; ++i)
//     {
//         wakers.emplace_back([&]() { wg.wakeOneIfNoActiveThreads(); });
//     }
//     for (auto& w : wakers)
//     {
//         w.join();
//     }
//
//     EXPECT_TRUE(spinWaitFor(done, kLongWait));
//     EXPECT_EQ(wokenCount.load(), 1u);
//
//     joinAll();
// }
//
// TEST_F(WaitGroupTest, StressConcurrentWakeOneCallsDoNotCorruptOrDeadlock)
// {
//     WaitGroup wg;
//     wg.init(kStressWaiterCount);
//     std::atomic<u32> parkedCount{ 0 };
//     std::atomic<u32> wokenCount{ 0 };
//
//     for (u32 i = 0; i < kStressWaiterCount; ++i)
//     {
//         track(std::thread(
//             [&]()
//             {
//                 parkedCount.fetch_add(1, std::memory_order_acq_rel);
//                 wg.wait();
//                 wokenCount.fetch_add(1, std::memory_order_acq_rel);
//             }));
//     }
//
//     spinWaitForCount(parkedCount, kStressWaiterCount);
//     std::this_thread::sleep_for(kShortWait);
//
//     // Fire many concurrent wakeOne calls with no wakeAll — the count check must
//     // ensure only one thread is released regardless of how many callers race.
//     std::vector<std::thread> racers;
//     racers.reserve(kStressThreadCount);
//     for (u32 i = 0; i < kStressThreadCount; ++i)
//     {
//         racers.emplace_back([&]() { wg.wakeOneIfNoActiveThreads(); });
//     }
//     for (auto& r : racers)
//     {
//         r.join();
//     }
//
//     // Multiple callers may all read numActiveThreads_ == 0 before any woken thread
//     // increments it back, causing more than one unparkOne to fire. This is permitted.
//     // Assert only that at least one thread woke and the state is not corrupted.
//     std::this_thread::sleep_for(kShortWait);
//     EXPECT_GE(wokenCount.load(), 1u);
//
//     // Drain the remaining parked threads and join before locals go out of scope.
//     wg.wakeAll();
//     spinWaitForCount(wokenCount, kStressWaiterCount);
//     joinAll();
// }

TEST_F(WaitGroupTest, wakeOneIfNoActiveThreadsRacesWithWait)
{
    static constexpr u32 kIterations = 100000;

    for (u32 i = 0; i < kIterations; ++i)
    {
        WaitGroup wg;
        wg.init(1);
        std::atomic<bool> hasWoken{ false };

        // Waiter thread: repeatedly try to park
        std::thread waiter(
            [&]()
            {
                wg.wait();
                hasWoken.store(true, std::memory_order_release);
            });

        // Waker thread: try to catch the waiter in the transition to parking.
        std::thread waker(
            [&]()
            {
                while (!hasWoken.load(std::memory_order_acquire))
                {
                    wg.wakeOneIfNoActiveThreads();
                    std::this_thread::yield();
                }
            });

        // If the race occurs and the wake-up is lost, hasWoken will never become true.
        bool success = spinWaitFor(hasWoken, std::chrono::seconds(10000));

        if (!success)
        {
            wg.wakeAll();
        }

        waiter.join();
        waker.join();

        ASSERT_TRUE(success) << "Lost wake-up detected at iteration " << i;
    }
}

} // namespace mk::cc
