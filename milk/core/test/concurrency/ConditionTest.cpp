#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <thread>
#include <vector>

#include <core/concurrency/ICondition.h>
#include <core/concurrency/ISharedMutex.h>

namespace mk::cc
{

// ---------------------------------------------------------------------------
// TC-1: Basic Wakeup
// ---------------------------------------------------------------------------

TEST(Condition, NotifyOneWakesSingleWaiter)
{
    SharedMutex            mutex;
    Condition<SharedMutex> cond;
    ConditionResult        result = ConditionResult::eTimeout;
    bool                   ready = false;
    std::atomic<int>       waitingCount = { 0 };

    std::thread waiter(
        [&]()
        {
            mutex.lock();
            waitingCount.fetch_add(1);
            while (!ready)
            {
                result = cond.wait(mutex, Condition<SharedMutex>::kForever);
            }
            mutex.unlock();
        });

    while (waitingCount.load() < 1)
    {
        std::this_thread::yield();
    }
    mutex.lock();
    ready = true;
    mutex.unlock();
    cond.notifyOne();
    waiter.join();

    EXPECT_EQ(result, ConditionResult::eNoTimeout);
}

TEST(Condition, NotifyAllWakesAllWaiters)
{
    constexpr int          N = 8;
    SharedMutex            mutex;
    Condition<SharedMutex> cond;
    bool                   ready = false;
    std::atomic<int>       waitingCount = { 0 };
    std::atomic<int>       woken = { 0 };

    std::vector<std::thread> threads;
    threads.reserve(N);
    for (int i = 0; i < N; ++i)
    {
        threads.emplace_back(
            [&]()
            {
                mutex.lock();
                waitingCount.fetch_add(1);
                while (!ready)
                {
                    cond.wait(mutex, Condition<SharedMutex>::kForever);
                }
                woken.fetch_add(1);
                mutex.unlock();
            });
    }

    while (waitingCount.load() < N)
    {
        std::this_thread::yield();
    }
    mutex.lock();
    ready = true;
    mutex.unlock();
    cond.notifyAll();

    for (auto& t : threads)
    {
        t.join();
    }

    EXPECT_EQ(woken.load(), N);
}

TEST(Condition, NotifyOneWakesExactlyOne)
{
    constexpr int          N = 4;
    SharedMutex            mutex;
    Condition<SharedMutex> cond;
    int                    tokensAvailable = 0; // protected by mutex
    std::atomic<int>       waitingCount = { 0 };
    std::atomic<int>       woken = { 0 };

    std::vector<std::thread> threads;
    for (int i = 0; i < N; ++i)
    {
        threads.emplace_back(
            [&]()
            {
                mutex.lock();
                waitingCount.fetch_add(1);
                while (tokensAvailable == 0)
                {
                    cond.wait(mutex, Condition<SharedMutex>::kForever);
                }
                --tokensAvailable;
                mutex.unlock();
                woken.fetch_add(1);
            });
    }

    while (waitingCount.load() < N)
    {
        std::this_thread::yield();
    }

    // Issue one token — only one thread may consume it
    mutex.lock();
    ++tokensAvailable;
    mutex.unlock();
    cond.notifyOne();

    std::this_thread::sleep_for(std::chrono::milliseconds(20));
    EXPECT_EQ(woken.load(), 1);

    // Drain the remaining waiters
    for (int i = 1; i < N; ++i)
    {
        mutex.lock();
        ++tokensAvailable;
        mutex.unlock();
        cond.notifyOne();
    }

    for (auto& t : threads)
    {
        t.join();
    }
}

// ---------------------------------------------------------------------------
// TC-2: Timeout Behavior
// ---------------------------------------------------------------------------

TEST(Condition, WaitExpiresAfterDuration)
{
    SharedMutex            mutex;
    Condition<SharedMutex> cond;

    // No notifier — wait() must return eTimeout on its own
    mutex.lock();
    auto            start = std::chrono::steady_clock::now();
    ConditionResult result = cond.wait(mutex, std::chrono::milliseconds(30));
    auto            elapsed = std::chrono::steady_clock::now() - start;
    mutex.unlock();

    EXPECT_EQ(result, ConditionResult::eTimeout);
    EXPECT_GE(elapsed, std::chrono::milliseconds(30));
}

TEST(Condition, ZeroDurationReturnsImmediately)
{
    SharedMutex            mutex;
    Condition<SharedMutex> cond;

    mutex.lock();
    ConditionResult result = cond.wait(mutex, SharedMutex::kImmediate);
    mutex.unlock();

    EXPECT_EQ(result, ConditionResult::eTimeout);
}

TEST(Condition, NotifyBeforeTimeoutReturnsNoTimeout)
{
    SharedMutex            mutex;
    Condition<SharedMutex> cond;
    ConditionResult        result = ConditionResult::eTimeout;
    bool                   ready = false;
    std::atomic<int>       waitingCount = { 0 };

    std::thread waiter(
        [&]()
        {
            mutex.lock();
            waitingCount.fetch_add(1);
            while (!ready)
            {
                result = cond.wait(mutex, std::chrono::milliseconds(200));
                if (result == ConditionResult::eTimeout)
                {
                    break;
                }
            }
            mutex.unlock();
        });

    while (waitingCount.load() < 1)
    {
        std::this_thread::yield();
    }
    mutex.lock();
    ready = true;
    mutex.unlock();
    cond.notifyOne();
    waiter.join();

    EXPECT_EQ(result, ConditionResult::eNoTimeout);
}

TEST(Condition, NotifyVsTimeoutRaceNeverHangs)
{
    // Test completes within the runner's deadline — no explicit assertion
    // other than no hang and no UB
    constexpr int          iterations = 10'000;
    SharedMutex            mutex;
    Condition<SharedMutex> cond;

    for (int i = 0; i < iterations; ++i)
    {
        bool ready = false;

        std::thread waiter(
            [&]()
            {
                mutex.lock();
                while (!ready)
                {
                    ConditionResult result =
                        cond.wait(mutex, std::chrono::microseconds(1));
                    if (result == ConditionResult::eTimeout)
                    {
                        break;
                    }
                }
                mutex.unlock();
            });

        mutex.lock();
        ready = true;
        mutex.unlock();
        cond.notifyOne();
        waiter.join();
    }
}

// ---------------------------------------------------------------------------
// TC-3: Notify Semantics
// ---------------------------------------------------------------------------

TEST(Condition, NotifyOneWithoutLockWakesWaiter)
{
    SharedMutex            mutex;
    Condition<SharedMutex> cond;
    ConditionResult        result = ConditionResult::eTimeout;
    bool                   ready = false;
    std::atomic<int>       waitingCount = { 0 };

    std::thread waiter(
        [&]()
        {
            mutex.lock();
            waitingCount.fetch_add(1);
            while (!ready)
            {
                result = cond.wait(mutex, Condition<SharedMutex>::kForever);
            }
            mutex.unlock();
        });

    while (waitingCount.load() < 1)
    {
        std::this_thread::yield();
    }
    mutex.lock();
    ready = true;
    mutex.unlock();
    // Deliberately notify WITHOUT holding mutex
    cond.notifyOne();
    waiter.join();

    EXPECT_EQ(result, ConditionResult::eNoTimeout);
}

TEST(Condition, NotifyOneWithLockHeldWakesWaiter)
{
    SharedMutex            mutex;
    Condition<SharedMutex> cond;
    ConditionResult        result = ConditionResult::eTimeout;
    bool                   ready = false;
    std::atomic<int>       waitingCount = { 0 };

    std::thread waiter(
        [&]()
        {
            mutex.lock();
            waitingCount.fetch_add(1);
            while (!ready)
            {
                result = cond.wait(mutex, Condition<SharedMutex>::kForever);
            }
            mutex.unlock();
        });

    while (waitingCount.load() < 1)
    {
        std::this_thread::yield();
    }
    mutex.lock();
    ready = true;
    cond.notifyOne(); // notify while still holding the lock
    mutex.unlock();
    waiter.join();

    EXPECT_EQ(result, ConditionResult::eNoTimeout);
}

TEST(Condition, NotifyWithNoWaitersIsSafe)
{
    SharedMutex            mutex;
    Condition<SharedMutex> cond;

    // Must not crash, deadlock, or corrupt state
    cond.notifyOne();
    cond.notifyAll();

    // Condition must still work correctly after the spurious notifies
    ConditionResult  result = ConditionResult::eTimeout;
    bool             ready = false;
    std::atomic<int> waitingCount = { 0 };

    std::thread waiter(
        [&]()
        {
            mutex.lock();
            waitingCount.fetch_add(1);
            while (!ready)
            {
                result = cond.wait(mutex, Condition<SharedMutex>::kForever);
            }
            mutex.unlock();
        });

    while (waitingCount.load() < 1)
    {
        std::this_thread::yield();
    }
    mutex.lock();
    ready = true;
    mutex.unlock();
    cond.notifyOne();
    waiter.join();

    EXPECT_EQ(result, ConditionResult::eNoTimeout);
}

TEST(Condition, RepeatedCyclesNoStateLeakage)
{
    constexpr int          cycles = 10'000;
    SharedMutex            mutex;
    Condition<SharedMutex> cond;

    for (int i = 0; i < cycles; ++i)
    {
        bool             ready = false;
        std::atomic<int> waitingCount = { 0 };

        std::thread waiter(
            [&]()
            {
                mutex.lock();
                waitingCount.fetch_add(1);
                while (!ready)
                {
                    cond.wait(mutex, Condition<SharedMutex>::kForever);
                }
                mutex.unlock();
            });

        while (waitingCount.load() < 1)
        {
            std::this_thread::yield();
        }
        mutex.lock();
        ready = true;
        mutex.unlock();
        cond.notifyOne();
        waiter.join();
    }
    // Completing all cycles without deadlock asserts no state leakage
}

// ---------------------------------------------------------------------------
// TC-4: Lock Contract
// ---------------------------------------------------------------------------

TEST(Condition, MutexReleasedWhileWaiting)
{
    SharedMutex            mutex;
    Condition<SharedMutex> cond;
    bool                   shouldWake = false;
    std::atomic<bool>      lockAcquiredByB = { false };
    std::atomic<int>       waitingCount = { 0 };

    std::thread waiter(
        [&]()
        {
            mutex.lock();
            waitingCount.fetch_add(1);
            while (!shouldWake)
            {
                cond.wait(mutex, Condition<SharedMutex>::kForever);
            }
            mutex.unlock();
        });

    std::thread observer(
        [&]()
        {
            while (waitingCount.load() < 1)
            {
                std::this_thread::yield();
            }
            // waiter is inside cond.wait() and must have released the mutex
            if (mutex.tryLock(std::chrono::milliseconds(50)))
            {
                lockAcquiredByB = true;
                shouldWake = true;
                mutex.unlock();
                cond.notifyOne();
            }
        });

    waiter.join();
    observer.join();

    EXPECT_TRUE(lockAcquiredByB.load());
}

TEST(Condition, MutexReacquiredOnWakeup)
{
    SharedMutex            mutex;
    Condition<SharedMutex> cond;
    bool                   shouldWake = false;
    std::atomic<bool>      waiterHoldsLock = { false };
    std::atomic<int>       waitingCount = { 0 };

    std::thread waiter(
        [&]()
        {
            mutex.lock();
            waitingCount.fetch_add(1);
            while (!shouldWake)
            {
                cond.wait(mutex, Condition<SharedMutex>::kForever);
            }
            // wait() must have re-acquired the mutex before returning
            waiterHoldsLock = true;
            mutex.unlock();
        });

    std::thread notifier(
        [&]()
        {
            while (waitingCount.load() < 1)
            {
                std::this_thread::yield();
            }
            mutex.lock();
            shouldWake = true;
            cond.notifyOne();
            mutex.unlock();
        });

    waiter.join();
    notifier.join();

    EXPECT_TRUE(waiterHoldsLock.load());
}

// ---------------------------------------------------------------------------
// TC-6: Multi-Waiter Concurrency
// ---------------------------------------------------------------------------

TEST(Condition, NotifyOneWakesOneAtATime)
{
    constexpr int          N = 4;
    SharedMutex            mutex;
    Condition<SharedMutex> cond;
    int                    tokensAvailable = 0; // protected by mutex
    std::atomic<int>       waitingCount = { 0 };
    std::atomic<int>       woken = { 0 };

    std::vector<std::thread> threads;
    for (int i = 0; i < N; ++i)
    {
        threads.emplace_back(
            [&]()
            {
                mutex.lock();
                waitingCount.fetch_add(1);
                while (tokensAvailable == 0)
                {
                    cond.wait(mutex, Condition<SharedMutex>::kForever);
                }
                --tokensAvailable;
                mutex.unlock();
                woken.fetch_add(1);
            });
    }

    while (waitingCount.load() < N)
    {
        std::this_thread::yield();
    }

    for (int i = 1; i <= N; ++i)
    {
        mutex.lock();
        ++tokensAvailable;
        mutex.unlock();
        cond.notifyOne();
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        EXPECT_EQ(woken.load(), i);
    }

    for (auto& t : threads)
    {
        t.join();
    }
}

TEST(Condition, PredicateLoopPatternWakesAll)
{
    constexpr int          N = 8;
    SharedMutex            mutex;
    Condition<SharedMutex> cond;
    bool                   ready = false;
    std::atomic<int>       waitingCount = { 0 };
    std::atomic<int>       woken = { 0 };

    std::vector<std::thread> threads;
    for (int i = 0; i < N; ++i)
    {
        threads.emplace_back(
            [&]()
            {
                mutex.lock();
                waitingCount.fetch_add(1);
                while (!ready)
                {
                    cond.wait(mutex, Condition<SharedMutex>::kForever);
                }
                woken.fetch_add(1);
                mutex.unlock();
            });
    }

    while (waitingCount.load() < N)
    {
        std::this_thread::yield();
    }
    mutex.lock();
    ready = true;
    mutex.unlock();
    cond.notifyAll();

    for (auto& t : threads)
    {
        t.join();
    }

    EXPECT_EQ(woken.load(), N);
}

// ---------------------------------------------------------------------------
// TC-7: Stress & Races
// ---------------------------------------------------------------------------

TEST(Condition, NoLostWakeupUnderRace)
{
    // Producer may set ready and call notifyOne() before the consumer reaches
    // wait(). The predicate loop guarantees the consumer never sleeps when the
    // condition is already met, so no wakeup can be lost.
    constexpr int          iterations = 1'000;
    SharedMutex            mutex;
    Condition<SharedMutex> cond;

    for (int i = 0; i < iterations; ++i)
    {
        bool ready = false;

        std::thread producer(
            [&]()
            {
                mutex.lock();
                ready = true;
                mutex.unlock();
                cond.notifyOne();
            });

        std::thread consumer(
            [&]()
            {
                mutex.lock();
                while (!ready)
                {
                    cond.wait(mutex, std::chrono::milliseconds(50));
                }
                mutex.unlock();
            });

        producer.join();
        consumer.join();
    }
}

TEST(Condition, ManyProducersManyConsumers)
{
    constexpr int          kProducers = 4;
    constexpr int          kConsumers = 4;
    SharedMutex            mutex;
    Condition<SharedMutex> cond;
    int                    tokensAvailable = 0; // protected by mutex
    std::atomic<int>       waitingCount = { 0 };
    std::atomic<int>       woken = { 0 };

    std::vector<std::thread> cThreads;
    cThreads.reserve(kConsumers);
    for (int i = 0; i < kConsumers; ++i)
    {
        cThreads.emplace_back(
            [&]()
            {
                mutex.lock();
                waitingCount.fetch_add(1);
                while (tokensAvailable == 0)
                {
                    cond.wait(mutex, Condition<SharedMutex>::kForever);
                }
                --tokensAvailable;
                woken.fetch_add(1);
                mutex.unlock();
            });
    }

    while (waitingCount.load() < kConsumers)
    {
        std::this_thread::yield();
    }

    std::vector<std::thread> pThreads;
    pThreads.reserve(kProducers);
    for (int i = 0; i < kProducers; ++i)
    {
        pThreads.emplace_back(
            [&]()
            {
                mutex.lock();
                ++tokensAvailable;
                mutex.unlock();
                cond.notifyOne();
            });
    }

    for (auto& t : pThreads)
    {
        t.join();
    }
    for (auto& t : cThreads)
    {
        t.join();
    }

    EXPECT_EQ(woken.load(), kConsumers);
}

} // namespace mk::cc
