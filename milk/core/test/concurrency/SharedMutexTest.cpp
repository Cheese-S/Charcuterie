#include <gtest/gtest.h>
#include <atomic>
#include <chrono>
#include <thread>
#include <vector>
#include <core/concurrency/ISharedMutex.h>
#include "core/IAssert.h"

namespace mk::cc
{

static constexpr int kStressThreads = 16;
static constexpr int kStressOps = 5000;

// ─────────────────────────────────────────────
// Helpers
// ─────────────────────────────────────────────

// RAII exclusive guard
struct ExclusiveGuard
{
    explicit ExclusiveGuard(SharedMutex& mtx): mtx_(mtx)
    {
        mtx_.lock();
    }
    ~ExclusiveGuard()
    {
        mtx_.unlock();
    }
    SharedMutex& mtx_;
};

// RAII shared guard
struct SharedGuard
{
    explicit SharedGuard(SharedMutex& mtx): mtx_(mtx)
    {
        mtx_.lockShared();
    }
    ~SharedGuard()
    {
        mtx_.unlockShared();
    }
    SharedMutex& mtx_;
};

// ═════════════════════════════════════════════
// Basic single-threaded correctness
// ═════════════════════════════════════════════

TEST(SharedMutex, ExclusiveLockUnlockDoesNotDeadlock)
{
    SharedMutex mtx;
    mtx.lock();
    mtx.unlock();
}

TEST(SharedMutex, SharedLockUnlockDoesNotDeadlock)
{
    SharedMutex mtx;
    mtx.lockShared();
    mtx.unlockShared();
}

TEST(SharedMutex, MultipleSequentialSharedLocksDoNotDeadlock)
{
    SharedMutex mtx;
    for (int i = 0; i < 8; ++i)
    {
        mtx.lockShared();
    }
    for (int i = 0; i < 8; ++i)
    {
        mtx.unlockShared();
    }
}

TEST(SharedMutex, RepeatedExclusiveLockUnlockDoesNotDeadlock)
{
    SharedMutex mtx;
    for (int i = 0; i < 1000; ++i)
    {
        mtx.lock();
        mtx.unlock();
    }
}

TEST(SharedMutex, RepeatedSharedLockUnlockDoesNotDeadlock)
{
    SharedMutex mtx;
    for (int i = 0; i < 1000; ++i)
    {
        mtx.lockShared();
        mtx.unlockShared();
    }
}

// ═════════════════════════════════════════════
// Exclusive lock — mutual exclusion
// ═════════════════════════════════════════════

// A plain int (no atomics) incremented inside the exclusive lock must
// reach the exact expected value — any lost update reveals a hole in
// mutual exclusion.
TEST(SharedMutex, ExclusiveLockProtectsCounter)
{
    SharedMutex mtx;
    int         counter = 0;

    std::vector<std::thread> threads;
    threads.reserve(kStressThreads);

    for (int i = 0; i < kStressThreads; ++i)
    {
        threads.emplace_back(
            [&]()
            {
                for (int j = 0; j < kStressOps; ++j)
                {
                    ExclusiveGuard guard(mtx);
                    ++counter;
                }
            });
    }

    for (auto& t : threads)
    {
        t.join();
    }

    EXPECT_EQ(counter, kStressThreads * kStressOps);
}

// Detect simultaneous entry: an atomic "inside" flag must never be
// non-zero when a second exclusive holder enters.
TEST(SharedMutex, OnlyOneExclusiveHolderAtATime)
{
    SharedMutex       mtx;
    std::atomic<int>  inside{ 0 };
    std::atomic<bool> violated{ false };

    std::vector<std::thread> threads;
    threads.reserve(kStressThreads);

    for (int i = 0; i < kStressThreads; ++i)
    {
        threads.emplace_back(
            [&]()
            {
                for (int j = 0; j < kStressOps; ++j)
                {
                    ExclusiveGuard guard(mtx);
                    int            prev = inside.fetch_add(1, std::memory_order_relaxed);
                    if (prev != 0)
                    {
                        violated.store(true, std::memory_order_relaxed);
                    }
                    inside.fetch_sub(1, std::memory_order_relaxed);
                }
            });
    }

    for (auto& t : threads)
    {
        t.join();
    }

    EXPECT_FALSE(violated.load());
}

// ═════════════════════════════════════════════
// Shared lock — concurrency between readers
// ═════════════════════════════════════════════

// Multiple readers must be able to hold the lock simultaneously —
// if they serialised, the test would time out.
TEST(SharedMutex, MultipleReadersRunConcurrently)
{
    constexpr int    kReaders = 8;
    SharedMutex      mtx;
    std::atomic<int> inside{ 0 };
    std::atomic<int> maxInside{ 0 };

    std::vector<std::thread> threads;
    threads.reserve(kReaders);

    for (int i = 0; i < kReaders; ++i)
    {
        threads.emplace_back(
            [&]()
            {
                mtx.lockShared();
                int val = inside.fetch_add(1, std::memory_order_acq_rel) + 1;

                // update max observed concurrency
                int expected = maxInside.load(std::memory_order_relaxed);
                while (val > expected &&
                       !maxInside.compare_exchange_weak(expected,
                                                        val,
                                                        std::memory_order_relaxed))
                {
                }

                std::this_thread::sleep_for(std::chrono::milliseconds(10));
                inside.fetch_sub(1, std::memory_order_relaxed);
                mtx.unlockShared();
            });
    }

    for (auto& t : threads)
    {
        t.join();
    }

    // At least two readers must have been inside simultaneously
    EXPECT_GT(maxInside.load(), 1);
}

// ═════════════════════════════════════════════
// Writer excludes all readers and vice-versa
// ═════════════════════════════════════════════

// While the exclusive lock is held no shared lock must be concurrently active.
TEST(SharedMutex, WriterExcludesAllReaders)
{
    SharedMutex       mtx;
    std::atomic<int>  readers{ 0 };
    std::atomic<bool> violated{ false };

    constexpr int kReaders = 8;
    constexpr int kWriters = 4;
    constexpr int kIterations = 500;

    std::vector<std::thread> threads;
    threads.reserve(kReaders + kWriters);

    for (int i = 0; i < kReaders; ++i)
    {
        threads.emplace_back(
            [&]()
            {
                for (int j = 0; j < kIterations; ++j)
                {
                    SharedGuard guard(mtx);
                    readers.fetch_add(1, std::memory_order_relaxed);
                    std::this_thread::yield();
                    readers.fetch_sub(1, std::memory_order_relaxed);
                }
            });
    }

    for (int i = 0; i < kWriters; ++i)
    {
        threads.emplace_back(
            [&]()
            {
                for (int j = 0; j < kIterations; ++j)
                {
                    ExclusiveGuard guard(mtx);
                    // While we hold the exclusive lock, no reader must be active
                    if (readers.load(std::memory_order_relaxed) != 0)
                    {
                        violated.store(true, std::memory_order_relaxed);
                    }
                }
            });
    }

    for (auto& t : threads)
    {
        t.join();
    }

    EXPECT_FALSE(violated.load());
}

// While any reader holds the lock, no exclusive holder must be active.
TEST(SharedMutex, ReadersExcludeWriter)
{
    SharedMutex       mtx;
    std::atomic<bool> writerInside{ false };
    std::atomic<bool> violated{ false };

    constexpr int kReaders = 8;
    constexpr int kIterations = 500;

    std::vector<std::thread> threads;
    threads.reserve(kReaders + 1);

    // Writer thread
    threads.emplace_back(
        [&]()
        {
            for (int j = 0; j < kIterations * kReaders; ++j)
            {
                ExclusiveGuard guard(mtx);
                writerInside.store(true, std::memory_order_relaxed);
                std::this_thread::yield();
                writerInside.store(false, std::memory_order_relaxed);
            }
        });

    for (int i = 0; i < kReaders; ++i)
    {
        threads.emplace_back(
            [&]()
            {
                for (int j = 0; j < kIterations; ++j)
                {
                    SharedGuard guard(mtx);
                    // While we hold a shared lock, no writer must be inside
                    if (writerInside.load(std::memory_order_relaxed))
                    {
                        violated.store(true, std::memory_order_relaxed);
                    }
                    std::this_thread::yield();
                }
            });
    }

    for (auto& t : threads)
    {
        t.join();
    }

    EXPECT_FALSE(violated.load());
}

// ═════════════════════════════════════════════
// drainShared — writer waits for existing readers
// ═════════════════════════════════════════════

// A reader holds the lock, then a writer tries to acquire. The writer
// must not return from lock() until the reader has released.
TEST(SharedMutex, WriterWaitsForExistingReaderToDrain)
{
    SharedMutex       mtx;
    std::atomic<bool> readerDone{ false };

    // Reader grabs shared lock first
    mtx.lockShared();

    std::thread writer(
        [&]()
        {
            mtx.lock(); // must block until reader releases
            // When we get here the reader must have finished
            EXPECT_TRUE(readerDone.load(std::memory_order_acquire));
            mtx.unlock();
        });

    std::this_thread::sleep_for(std::chrono::milliseconds(20));
    readerDone.store(true, std::memory_order_release);
    mtx.unlockShared();

    writer.join();
}

// ═════════════════════════════════════════════
// Reader waits for exclusive writer
// ═════════════════════════════════════════════

TEST(SharedMutex, ReaderWaitsForExclusiveWriter)
{
    SharedMutex       mtx;
    std::atomic<bool> writerDone{ false };

    mtx.lock();

    std::thread reader(
        [&]()
        {
            mtx.lockShared(); // must block until writer releases
            EXPECT_TRUE(writerDone.load(std::memory_order_acquire));
            mtx.unlockShared();
        });

    std::this_thread::sleep_for(std::chrono::milliseconds(20));
    writerDone.store(true, std::memory_order_release);
    mtx.unlock();

    reader.join();
}

// ═════════════════════════════════════════════
// Mixed read / write stress
// ═════════════════════════════════════════════

// Half threads are readers (shared), half writers (exclusive).
// A shared value written only under exclusive lock and read only under
// shared lock must never be observed in a torn state.
TEST(SharedMutex, MixedReadWriteStressNoDataRace)
{
    SharedMutex mtx;
    int         value = 0;
    int         counter = 0; // incremented exclusively, read shared

    constexpr int kReaders = kStressThreads / 2;
    constexpr int kWriters = kStressThreads / 2;
    constexpr int kIterations = kStressOps;

    std::atomic<bool> violated{ false };

    std::vector<std::thread> threads;
    threads.reserve(kReaders + kWriters);

    for (int i = 0; i < kWriters; ++i)
    {
        threads.emplace_back(
            [&]()
            {
                for (int j = 0; j < kIterations; ++j)
                {
                    ExclusiveGuard guard(mtx);
                    ++counter;
                    value = counter; // value always == counter under exclusive lock
                }
            });
    }

    for (int i = 0; i < kReaders; ++i)
    {
        threads.emplace_back(
            [&]()
            {
                for (int j = 0; j < kIterations; ++j)
                {
                    SharedGuard guard(mtx);
                    // counter == value must hold — they are always written together
                    if (value != counter)
                    {
                        violated.store(true, std::memory_order_relaxed);
                    }
                }
            });
    }

    for (auto& t : threads)
    {
        t.join();
    }

    EXPECT_FALSE(violated.load());
    EXPECT_EQ(counter, kWriters * kIterations);
}

// ═════════════════════════════════════════════
// Exclusive lock after all readers release
// ═════════════════════════════════════════════

TEST(SharedMutex, ExclusiveLockSucceedsAfterAllReadersRelease)
{
    SharedMutex      mtx;
    constexpr int    kReaders = 4;
    std::atomic<int> released{ 0 };

    std::vector<std::thread> readers;
    readers.reserve(kReaders);

    for (int i = 0; i < kReaders; ++i)
    {
        readers.emplace_back(
            [&]()
            {
                mtx.lockShared();
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
                released.fetch_add(1, std::memory_order_release);
                mtx.unlockShared();
            });
    }

    // Give readers time to all enter
    std::this_thread::sleep_for(std::chrono::milliseconds(5));

    mtx.lock(); // must drain all kReaders before returning
    EXPECT_EQ(released.load(std::memory_order_acquire), kReaders);
    mtx.unlock();

    for (auto& t : readers)
    {
        t.join();
    }
}

// ═════════════════════════════════════════════
// tryLock — immediate (kImmediate / default)
// ═════════════════════════════════════════════

// tryLock must succeed (return true) when no one else holds the mutex.
TEST(SharedMutex, TryLockSucceedsWhenFree)
{
    SharedMutex mtx;
    EXPECT_TRUE(mtx.tryLock());
    mtx.unlock();
}

// tryLock must fail immediately if another thread holds the exclusive lock.
TEST(SharedMutex, TryLockFailsWhenExclusiveLockHeld)
{
    SharedMutex mtx;
    mtx.lock();

    std::atomic<bool> result{ true };
    std::thread t([&]() { result.store(mtx.tryLock(), std::memory_order_relaxed); });
    t.join();

    EXPECT_FALSE(result.load());
    mtx.unlock();
}

// After a successful tryLock the mutex is exclusively held; a second
// tryLock from another thread must fail.
TEST(SharedMutex, TryLockHoldsExclusivelyAfterSuccess)
{
    SharedMutex       mtx;
    std::atomic<bool> violated{ false };

    ASSERT_TRUE(mtx.tryLock());

    std::thread t(
        [&]()
        {
            // Neither exclusive nor shared acquisition should succeed.
            if (mtx.tryLock())
            {
                violated.store(true, std::memory_order_relaxed);
                mtx.unlock();
            }
            if (mtx.tryLockShared())
            {
                violated.store(true, std::memory_order_relaxed);
                mtx.unlockShared();
            }
        });
    t.join();

    EXPECT_FALSE(violated.load());
    mtx.unlock();
}

// ═════════════════════════════════════════════
// tryLock — with timeout
// ═════════════════════════════════════════════

// tryLock with a finite timeout must return true when the mutex becomes
// available within the window.
TEST(SharedMutex, TryLockWithTimeoutSucceedsWhenLockBecomesAvailable)
{
    SharedMutex mtx;
    mtx.lock();

    std::atomic<bool> result{ false };
    std::thread       t(
        [&]()
        {
            // Allow up to 500 ms — the main thread releases after ~20 ms.
            result.store(mtx.tryLock(std::chrono::milliseconds(500)),
                         std::memory_order_relaxed);
            if (result.load(std::memory_order_relaxed))
            {
                mtx.unlock();
            }
        });

    std::this_thread::sleep_for(std::chrono::milliseconds(20));
    mtx.unlock(); // release so the waiting thread can acquire

    t.join();
    EXPECT_TRUE(result.load());
}

// tryLock with a finite timeout must return false when the mutex is never
// released within the window.
TEST(SharedMutex, TryLockWithTimeoutFailsWhenTimeoutExpires)
{
    SharedMutex mtx;
    mtx.lock(); // held for the entire duration of the test

    std::atomic<bool> result{ true };
    std::thread       t(
        [&]()
        {
            result.store(mtx.tryLock(std::chrono::milliseconds(30)),
                         std::memory_order_relaxed);
        });
    t.join();

    EXPECT_FALSE(result.load());
    mtx.unlock();
}

// tryLock(kForever) must block until the lock is actually released,
// behaving identically to lock().
TEST(SharedMutex, TryLockForeverBehavesLikeLock)
{
    SharedMutex       mtx;
    std::atomic<bool> writerDone{ false };

    mtx.lock();

    std::thread t(
        [&]()
        {
            // kForever — must not return until main releases
            EXPECT_TRUE(mtx.tryLock(util::kForever));
            EXPECT_TRUE(writerDone.load(std::memory_order_acquire));
            mtx.unlock();
        });

    std::this_thread::sleep_for(std::chrono::milliseconds(20));
    writerDone.store(true, std::memory_order_release);
    mtx.unlock();

    t.join();
}

// ═════════════════════════════════════════════
// tryLockShared — immediate (kImmediate / default)
// ═════════════════════════════════════════════

// tryLockShared must succeed (return true) when no one else holds the mutex.
TEST(SharedMutex, TryLockSharedSucceedsWhenFree)
{
    SharedMutex mtx;
    EXPECT_TRUE(mtx.tryLockShared());
    mtx.unlockShared();
}

// tryLockShared must fail immediately when an exclusive lock is held.
TEST(SharedMutex, TryLockSharedFailsWhenExclusiveLockHeld)
{
    SharedMutex mtx;
    mtx.lock();

    std::atomic<bool> result{ true };
    std::thread       t([&]()
                  { result.store(mtx.tryLockShared(), std::memory_order_relaxed); });
    t.join();

    EXPECT_FALSE(result.load());
    mtx.unlock();
}

// Multiple concurrent readers must all succeed with tryLockShared —
// shared access is not exclusive.
TEST(SharedMutex, TryLockSharedSucceedsWhileOtherReadersHoldLock)
{
    SharedMutex mtx;
    mtx.lockShared();

    // A second reader via tryLockShared must also succeed.
    std::atomic<bool> result{ false };
    std::thread       t(
        [&]()
        {
            result.store(mtx.tryLockShared(), std::memory_order_relaxed);
            if (result.load(std::memory_order_relaxed))
            {
                mtx.unlockShared();
            }
        });
    t.join();

    EXPECT_TRUE(result.load());
    mtx.unlockShared(); // release the first reader
}

// ═════════════════════════════════════════════
// tryLockShared — with timeout
// ═════════════════════════════════════════════

// tryLockShared with a finite timeout must return true when the exclusive
// lock is released within the window.
TEST(SharedMutex, TryLockSharedWithTimeoutSucceedsWhenLockBecomesAvailable)
{
    SharedMutex mtx;
    mtx.lock(); // exclusive — blocks all readers

    std::atomic<bool> result{ false };
    std::thread       t(
        [&]()
        {
            result.store(mtx.tryLockShared(std::chrono::milliseconds(500)),
                         std::memory_order_relaxed);
            if (result.load(std::memory_order_relaxed))
            {
                mtx.unlockShared();
            }
        });

    std::this_thread::sleep_for(std::chrono::milliseconds(20));
    mtx.unlock();

    t.join();
    EXPECT_TRUE(result.load());
}

// tryLockShared with a finite timeout must return false when the exclusive
// lock is never released within the window.
TEST(SharedMutex, TryLockSharedWithTimeoutFailsWhenTimeoutExpires)
{
    SharedMutex mtx;
    mtx.lock();

    std::atomic<bool> result{ true };
    std::thread       t(
        [&]()
        {
            result.store(mtx.tryLockShared(std::chrono::milliseconds(30)),
                         std::memory_order_relaxed);
        });
    t.join();

    EXPECT_FALSE(result.load());
    mtx.unlock();
}

// tryLockShared(kForever) must block until the exclusive lock is released,
// behaving identically to lockShared().
TEST(SharedMutex, TryLockSharedForeverBehavesLikeLockShared)
{
    SharedMutex       mtx;
    std::atomic<bool> writerDone{ false };

    mtx.lock();

    std::thread t(
        [&]()
        {
            EXPECT_TRUE(mtx.tryLockShared(util::kForever));
            EXPECT_TRUE(writerDone.load(std::memory_order_acquire));
            mtx.unlockShared();
        });

    std::this_thread::sleep_for(std::chrono::milliseconds(20));
    writerDone.store(true, std::memory_order_release);
    mtx.unlock();

    t.join();
}

// ═════════════════════════════════════════════
// tryLock / tryLockShared — mixed stress
// ═════════════════════════════════════════════

// Mix of lock(), tryLock(), lockShared(), and tryLockShared() under load.
// A plain counter incremented only under exclusive acquisition must reach
// the exact expected value.  Reader threads verify value == counter while
// holding a shared lock — invariant violations are flagged atomically.
TEST(SharedMutex, TryVariantsMixedStressNoDataRace)
{
    SharedMutex       mtx;
    int               counter = 0;
    int               value = 0;
    std::atomic<bool> violated{ false };

    constexpr int kWriters = kStressThreads / 2;
    constexpr int kReaders = kStressThreads / 2;
    constexpr int kIterations = kStressOps;

    std::vector<std::thread> threads;
    threads.reserve(kWriters + kReaders);

    // Writers: alternate between lock() and tryLock(kForever) so that
    // every increment is guaranteed to complete.
    for (int i = 0; i < kWriters; ++i)
    {
        threads.emplace_back(
            [&, i]()
            {
                for (int j = 0; j < kIterations; ++j)
                {
                    if ((i + j) % 2 == 0)
                    {
                        ExclusiveGuard guard(mtx);
                        ++counter;
                        value = counter;
                    }
                    else
                    {
                        while (!mtx.tryLock(std::chrono::milliseconds(1)))
                        {
                        }
                        ++counter;
                        value = counter;
                        mtx.unlock();
                    }
                }
            });
    }

    // Readers: alternate between lockShared() and tryLockShared(kForever).
    for (int i = 0; i < kReaders; ++i)
    {
        threads.emplace_back(
            [&, i]()
            {
                for (int j = 0; j < kIterations; ++j)
                {
                    if ((i + j) % 2 == 0)
                    {
                        SharedGuard guard(mtx);
                        if (value != counter)
                        {
                            violated.store(true, std::memory_order_relaxed);
                        }
                    }
                    else
                    {
                        while (!mtx.tryLockShared(std::chrono::milliseconds(1)))
                        {
                        }
                        if (value != counter)
                        {
                            violated.store(true, std::memory_order_relaxed);
                        }
                        mtx.unlockShared();
                    }
                }
            });
    }

    for (auto& t : threads)
    {
        t.join();
    }

    EXPECT_FALSE(violated.load());
    EXPECT_EQ(counter, kWriters * kIterations);
}

} // namespace mk::cc
