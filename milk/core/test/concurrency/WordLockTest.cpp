#include <gtest/gtest.h>
#include <thread>
#include <vector>
#include <atomic>
#include <core/concurrency/IWordLock.h>

namespace mk::cc
{

// ─────────────────────────────────────────────
// Basic single-threaded behaviour
// ─────────────────────────────────────────────

TEST(WordLock, LockAndUnlockDoesNotDeadlock)
{
    WordLock lk;
    lk.lock();
    lk.unlock();
}

TEST(WordLock, RepeatedLockUnlockDoesNotDeadlock)
{
    WordLock lk;
    for (int i = 0; i < 1000; ++i)
    {
        lk.lock();
        lk.unlock();
    }
}

// ─────────────────────────────────────────────
// Mutual exclusion
// ─────────────────────────────────────────────

// A counter incremented without synchronisation inside the lock
// must reach the expected value with no lost updates.
TEST(WordLock, MutualExclusionProtectsCounter)
{
    static constexpr int kNumThreads = 8;
    static constexpr int kOpsPerThread = 10000;

    WordLock lk;
    int      counter = 0;

    std::vector<std::thread> threads;
    threads.reserve(kNumThreads);

    for (int i = 0; i < kNumThreads; ++i)
    {
        threads.emplace_back(
            [&lk, &counter]()
            {
                for (int j = 0; j < kOpsPerThread; ++j)
                {
                    lk.lock();
                    ++counter;
                    lk.unlock();
                }
            });
    }

    for (auto& t : threads)
    {
        t.join();
    }

    EXPECT_EQ(counter, kNumThreads * kOpsPerThread);
}

// Only one thread must be inside the critical section at any moment.
// We detect violations by checking that a "inside" flag is never set
// when we enter.
TEST(WordLock, OnlyOneThreadInsideCriticalSection)
{
    static constexpr int kNumThreads = 16;
    static constexpr int kOpsPerThread = 5000;

    WordLock          lk;
    std::atomic<int>  inside{ 0 };
    std::atomic<bool> violated{ false };

    std::vector<std::thread> threads;
    threads.reserve(kNumThreads);

    for (int i = 0; i < kNumThreads; ++i)
    {
        threads.emplace_back(
            [&]()
            {
                for (int j = 0; j < kOpsPerThread; ++j)
                {
                    lk.lock();
                    int prev = inside.fetch_add(1, std::memory_order_relaxed);
                    if (prev != 0)
                    {
                        violated.store(true, std::memory_order_relaxed);
                    }
                    inside.fetch_sub(1, std::memory_order_relaxed);
                    lk.unlock();
                }
            });
    }

    for (auto& t : threads)
    {
        t.join();
    }

    EXPECT_FALSE(violated.load());
}

// ─────────────────────────────────────────────
// Contention — slow path (queue) exercised
// ─────────────────────────────────────────────

// With many threads hammering the lock simultaneously the slow path
// (lockSlow / unlockSlow queue logic) will be exercised. The counter
// must still reach the exact expected value.
TEST(WordLock, HighContentionSlowPathCounter)
{
    static constexpr int kNumThreads = 32;
    static constexpr int kOpsPerThread = 5000;

    WordLock lk;
    int      counter = 0;

    std::vector<std::thread> threads;
    threads.reserve(kNumThreads);

    for (int i = 0; i < kNumThreads; ++i)
    {
        threads.emplace_back(
            [&lk, &counter]()
            {
                for (int j = 0; j < kOpsPerThread; ++j)
                {
                    lk.lock();
                    ++counter;
                    lk.unlock();
                }
            });
    }

    for (auto& t : threads)
    {
        t.join();
    }

    EXPECT_EQ(counter, kNumThreads * kOpsPerThread);
}

// ─────────────────────────────────────────────
// Producer / consumer — ordered handoff
// ─────────────────────────────────────────────

// One thread writes a value under the lock; another reads it.
// The reader must always observe the fully written value, never a torn read.
TEST(WordLock, ProducerConsumerHandoff)
{
    static constexpr int kIterations = 20000;

    WordLock          lk;
    int               sharedValue = 0;
    std::atomic<bool> produced{ false };

    std::atomic<int> mismatches{ 0 };

    std::thread producer(
        [&]()
        {
            for (int i = 1; i <= kIterations; ++i)
            {
                // Spin until consumer has taken the previous item
                while (produced.load(std::memory_order_acquire))
                {
                }
                lk.lock();
                sharedValue = i;
                lk.unlock();
                produced.store(true, std::memory_order_release);
            }
        });

    std::thread consumer(
        [&]()
        {
            for (int i = 1; i <= kIterations; ++i)
            {
                while (!produced.load(std::memory_order_acquire))
                {
                }
                lk.lock();
                int observed = sharedValue;
                lk.unlock();
                if (observed != i)
                {
                    mismatches.fetch_add(1, std::memory_order_relaxed);
                }
                produced.store(false, std::memory_order_release);
            }
        });

    producer.join();
    consumer.join();

    EXPECT_EQ(mismatches.load(), 0);
}
} // namespace mk::cc
