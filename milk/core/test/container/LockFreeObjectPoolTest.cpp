#include <gtest/gtest.h>

#include <algorithm>
#include <atomic>
#include <barrier>
#include <array>
#include <cstring>
#include <latch>
#include <set>
#include <thread>
#include <vector>

#include <core/container/ILockFreeObjectPool.h>
#include <core/IMemory.h>

namespace mk::test
{
using std::vector;

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

struct Widget
{
    explicit Widget(int v): value(v) {}
    int  value;
    bool alive{ true };

    inline static std::atomic<int> constructions_{ 0 };
    inline static std::atomic<int> destructions_{ 0 };

    static void resetCounters()
    {
        constructions_ = 0;
        destructions_ = 0;
    }
};

struct TrackedWidget
{
    explicit TrackedWidget(int v): value(v)
    {
        ++Widget::constructions_;
    }
    ~TrackedWidget()
    {
        ++Widget::destructions_;
    }
    int value;
};

// Canary-guarded widget. A unique fill pattern is written on construction and
// verified before release — corruption from overlapping nodes or double-acquires
// shows up as a stamp mismatch.
struct GuardedWidget
{
    static constexpr u32 kLiveMagic = 0xCAFEBABE;
    static constexpr u32 kDeadMagic = 0xDEADBEEF;

    explicit GuardedWidget(int v): value(v), magic(kLiveMagic)
    {
        ++constructions_;
    }

    ~GuardedWidget()
    {
        EXPECT_EQ(magic, kLiveMagic)
            << "Magic corrupt on destruction (value=" << value << ")";
        magic = kDeadMagic;
        ++destructions_;
    }

    void stamp(int tid)
    {
        std::memset(payload, static_cast<unsigned char>(tid & 0xFF), sizeof(payload));
    }

    bool verifyStamp(int tid) const
    {
        const unsigned char expected = static_cast<unsigned char>(tid & 0xFF);
        for (auto b : payload)
        {
            if (b != expected)
            {
                return false;
            }
        }
        return true;
    }

    int           value;
    u32           magic{};
    unsigned char payload[48]{};

    inline static std::atomic<int> constructions_{ 0 };
    inline static std::atomic<int> destructions_{ 0 };

    static void resetCounters()
    {
        constructions_ = 0;
        destructions_ = 0;
    }
};

// ---------------------------------------------------------------------------
// Type aliases
// ---------------------------------------------------------------------------

// Single-chunk pools (maxChunksCount=1). Behaviour is identical to the old
// three-parameter template with MaxObjects == NumOfObjectsPerChunk.
using SmallPool = LockFreeObjectPool<Widget, 4>;
using TrackedPool = LockFreeObjectPool<TrackedWidget, 8>;

// 64-slot-per-chunk pool used by the existing stress suite.
using StressPool = LockFreeObjectPool<GuardedWidget, 64>;

// 16-slot-per-chunk pool used by the chunk-allocation tests so that multiple
// chunks are exercised without requiring hundreds of objects.
using GrowStress = LockFreeObjectPool<GuardedWidget, 16>;

// ---------------------------------------------------------------------------
// Shared helpers for GuardedWidget pools
// ---------------------------------------------------------------------------

template<typename PoolT>
static vector<GuardedWidget*> drainAll(PoolT& pool)
{
    std::vector<GuardedWidget*> out;
    while (GuardedWidget* w = pool.tryAcquire(-1))
    {
        out.push_back(w);
    }
    return out;
}

template<typename PoolT>
static void returnAll(PoolT& pool, std::vector<GuardedWidget*>& ptrs)
{
    for (GuardedWidget* w : ptrs)
    {
        pool.release(w);
    }
    ptrs.clear();
}

static constexpr usize kStressChunkSize = 64;

// ═══════════════════════════════════════════════════════════════════════════
// Basic construction
// ═══════════════════════════════════════════════════════════════════════════

TEST(LockFreeObjectPool, ConstructsWithoutCrash)
{
    EXPECT_NO_FATAL_FAILURE({ SmallPool pool(1); });
}

// ═══════════════════════════════════════════════════════════════════════════
// Single-threaded acquire / release
// ═══════════════════════════════════════════════════════════════════════════

TEST(LockFreeObjectPool, AcquireReturnsNonNull)
{
    SmallPool pool(1);
    Widget*   w = pool.tryAcquire(42);
    ASSERT_NE(w, nullptr);
    EXPECT_EQ(w->value, 42);
    pool.release(w);
}

TEST(LockFreeObjectPool, AcquireExhaustsPool)
{
    SmallPool            pool(1);
    std::vector<Widget*> acquired;

    for (int i = 0; i < 4; ++i)
    {
        Widget* w = pool.tryAcquire(i);
        ASSERT_NE(w, nullptr) << "Expected non-null at slot " << i;
        acquired.push_back(w);
    }

    Widget* overflow = pool.tryAcquire(99);
    EXPECT_EQ(overflow, nullptr);

    for (Widget* w : acquired)
    {
        pool.release(w);
    }
}

TEST(LockFreeObjectPool, AcquiredPointersAreDistinct)
{
    SmallPool         pool(1);
    std::set<Widget*> ptrs;

    for (int i = 0; i < 4; ++i)
    {
        Widget* w = pool.tryAcquire(i);
        ASSERT_NE(w, nullptr);
        EXPECT_TRUE(ptrs.insert(w).second) << "Duplicate pointer returned";
    }

    for (Widget* w : ptrs)
    {
        pool.release(w);
    }
}

TEST(LockFreeObjectPool, ReleaseAndReacquireRecyclesNode)
{
    SmallPool pool(1);

    Widget* first = pool.tryAcquire(1);
    ASSERT_NE(first, nullptr);
    Widget* addr = first;

    std::vector<Widget*> rest;
    rest.reserve(3);
    for (int i = 0; i < 3; ++i)
    {
        rest.push_back(pool.tryAcquire(i + 10));
    }

    pool.release(first);
    Widget* recycled = pool.tryAcquire(99);
    EXPECT_EQ(recycled, addr);

    pool.release(recycled);
    for (Widget* w : rest)
    {
        pool.release(w);
    }
}

TEST(LockFreeObjectPool, ReleaseOwnedPointerDoesNotAssert)
{
    SmallPool pool(1);
    Widget*   w = pool.tryAcquire(1);
    ASSERT_NE(w, nullptr);
    // release() returns void; the only guarantee is it must not crash or assert
    // for a pointer that was validly acquired from this pool.
    EXPECT_NO_FATAL_FAILURE(pool.release(w));
}

TEST(LockFreeObjectPool, ConstructorArgsForwarded)
{
    SmallPool pool(1);
    Widget*   w = pool.tryAcquire(77);
    ASSERT_NE(w, nullptr);
    EXPECT_EQ(w->value, 77);
    pool.release(w);
}

// ═══════════════════════════════════════════════════════════════════════════
// Destructor / constructor tracking
// ═══════════════════════════════════════════════════════════════════════════

TEST(LockFreeObjectPool, DestructorCalledOnRelease)
{
    Widget::resetCounters();
    TrackedPool pool(1);

    TrackedWidget* w = pool.tryAcquire(1);
    ASSERT_NE(w, nullptr);
    EXPECT_EQ(Widget::constructions_.load(), 1);
    EXPECT_EQ(Widget::destructions_.load(), 0);

    pool.release(w);
    EXPECT_EQ(Widget::destructions_.load(), 1);
}

TEST(LockFreeObjectPool, ConstructorCalledOnEachAcquire)
{
    Widget::resetCounters();
    TrackedPool pool(1);

    TrackedWidget* a = pool.tryAcquire(1);
    TrackedWidget* b = pool.tryAcquire(2);
    EXPECT_EQ(Widget::constructions_.load(), 2);

    pool.release(a);
    pool.release(b);
    EXPECT_EQ(Widget::destructions_.load(), 2);

    TrackedWidget* c = pool.tryAcquire(3);
    TrackedWidget* d = pool.tryAcquire(4);
    EXPECT_EQ(Widget::constructions_.load(), 4);

    pool.release(c);
    pool.release(d);
}

// ═══════════════════════════════════════════════════════════════════════════
// Full drain → release → refill cycle
// ═══════════════════════════════════════════════════════════════════════════

TEST(LockFreeObjectPool, FullDrainAndRefill)
{
    constexpr int kSize = 4;
    SmallPool     pool(1);
    Widget*       slots[kSize]{};

    for (int i = 0; i < kSize; ++i)
    {
        slots[i] = pool.tryAcquire(i);
        ASSERT_NE(slots[i], nullptr);
    }
    EXPECT_EQ(pool.tryAcquire(0), nullptr);

    for (int i = 0; i < kSize; ++i)
    {
        pool.release(slots[i]);
        Widget* w = pool.tryAcquire(i * 10);
        ASSERT_NE(w, nullptr) << "Re-acquire failed at index " << i;
        slots[i] = w;
    }

    for (auto& slot : slots)
    {
        pool.release(slot);
    }
}

// ═══════════════════════════════════════════════════════════════════════════
// Multi-threaded stress tests
// ═══════════════════════════════════════════════════════════════════════════

TEST(LockFreeObjectPool, ConcurrentAcquireReleaseDoesNotCrash)
{
    LockFreeObjectPool<Widget, 64> pool(1);

    constexpr int kThreads = 8;
    constexpr int kIterations = 2'000;

    std::atomic<int> acquired{ 0 };
    std::atomic<int> failures{ 0 };

    auto worker = [&]()
    {
        for (int i = 0; i < kIterations; ++i)
        {
            Widget* w = pool.tryAcquire(i);
            if (w)
            {
                ++acquired;
                w->value = i;
                [[maybe_unused]] volatile int v = w->value;
                pool.release(w);
            }
            else
            {
                ++failures;
            }
        }
    };

    std::vector<std::thread> threads;
    threads.reserve(kThreads);
    for (int t = 0; t < kThreads; ++t)
    {
        threads.emplace_back(worker);
    }
    for (auto& t : threads)
    {
        t.join();
    }

    int drained = 0;
    while (pool.tryAcquire(0))
    {
        ++drained;
    }

    EXPECT_EQ(drained, 64);
    EXPECT_GT(acquired.load(), 0);
}

TEST(LockFreeObjectPool, ProducerConsumerPattern)
{
    LockFreeObjectPool<Widget, 32> pool(1);

    constexpr int kRounds = 500;

    std::atomic<Widget*> handoff{ nullptr };
    std::atomic<bool>    done{ false };

    std::thread producer(
        [&]()
        {
            for (int i = 0; i < kRounds; ++i)
            {
                Widget* w = nullptr;
                while (!(w = pool.tryAcquire(i)))
                {
                    std::this_thread::yield();
                }

                Widget* expected = nullptr;
                while (!handoff.compare_exchange_weak(expected,
                                                      w,
                                                      std::memory_order_release))
                {
                    expected = nullptr;
                    std::this_thread::yield();
                }
            }
            done.store(true, std::memory_order_release);
        });

    std::thread consumer(
        [&]()
        {
            int received = 0;
            while (!done.load(std::memory_order_acquire) || received < kRounds)
            {
                Widget* w = handoff.exchange(nullptr, std::memory_order_acquire);
                if (w)
                {
                    pool.release(w);
                    ++received;
                }
                else
                {
                    std::this_thread::yield();
                }
            }
        });

    producer.join();
    consumer.join();

    int count = 0;
    while (pool.tryAcquire(0))
    {
        ++count;
    }
    EXPECT_EQ(count, 32);
}

// ═══════════════════════════════════════════════════════════════════════════
// Stress tests
// ═══════════════════════════════════════════════════════════════════════════

// ---------------------------------------------------------------------------
// Stress 1: hammer — every thread acquires and immediately releases.
//   Verifies no double-acquire and full node recovery.
// ---------------------------------------------------------------------------

TEST(LockFreeObjectPoolStress, HammerAcquireRelease)
{
    constexpr int kThreads = 16;
    constexpr int kIters = 20'000;

    StressPool pool(1);

    auto           all = drainAll(pool);
    GuardedWidget* base =
        all.empty() ? nullptr : *std::min_element(all.begin(), all.end());
    returnAll(pool, all);
    ASSERT_NE(base, nullptr);

    std::array<std::atomic<int>, kStressChunkSize> inUse{};
    for (auto& f : inUse)
    {
        f.store(0, std::memory_order_relaxed);
    }

    std::atomic<int>  acquired{ 0 };
    std::atomic<bool> doubleAcquire{ false };

    std::latch startLine(kThreads + 1);

    auto worker = [&](int tid)
    {
        startLine.arrive_and_wait();

        for (int i = 0; i < kIters; ++i)
        {
            GuardedWidget* w = pool.tryAcquire(tid);
            if (!w)
            {
                continue;
            }

            ++acquired;

            const std::ptrdiff_t idx = w - base;
            if (idx >= 0 && static_cast<usize>(idx) < kStressChunkSize)
            {
                if (inUse[idx].exchange(1, std::memory_order_acq_rel) != 0)
                {
                    doubleAcquire.store(true, std::memory_order_relaxed);
                }

                w->stamp(tid);
                EXPECT_TRUE(w->verifyStamp(tid))
                    << "Stamp mismatch — another thread wrote to our slot";

                inUse[idx].store(0, std::memory_order_release);
            }

            pool.release(w);
        }
    };

    std::vector<std::thread> threads;
    threads.reserve(kThreads);
    for (int t = 0; t < kThreads; ++t)
    {
        threads.emplace_back(worker, t);
    }

    startLine.arrive_and_wait();
    for (auto& t : threads)
    {
        t.join();
    }

    EXPECT_FALSE(doubleAcquire.load())
        << "Same slot acquired by two threads simultaneously";
    EXPECT_GT(acquired.load(), 0) << "No thread ever acquired a slot";

    auto remaining = drainAll(pool);
    EXPECT_EQ(static_cast<int>(remaining.size()), static_cast<int>(kStressChunkSize))
        << "Pool lost " << (kStressChunkSize - remaining.size()) << " node(s)";
    returnAll(pool, remaining);
}

// ---------------------------------------------------------------------------
// Stress 2: exhaustion race — threads race to drain then refill each round.
// ---------------------------------------------------------------------------

TEST(LockFreeObjectPoolStress, ConcurrentExhaustionAndRefill)
{
    constexpr int kThreads = 8;
    constexpr int kRounds = 500;

    StressPool pool(1);

    std::barrier roundBarrier(kThreads);

    auto worker = [&]()
    {
        for (int round = 0; round < kRounds; ++round)
        {
            roundBarrier.arrive_and_wait();

            std::vector<GuardedWidget*> held;
            for (int attempt = 0; attempt < static_cast<int>(kStressChunkSize) * 2;
                 ++attempt)
            {
                GuardedWidget* w = pool.tryAcquire(round);
                if (w)
                {
                    held.push_back(w);
                }
            }

            std::this_thread::yield();

            for (GuardedWidget* w : held)
            {
                pool.release(w);
            }
        }
    };

    std::vector<std::thread> threads;
    threads.reserve(kThreads);
    for (int t = 0; t < kThreads; ++t)
    {
        threads.emplace_back(worker);
    }
    for (auto& t : threads)
    {
        t.join();
    }

    auto remaining = drainAll(pool);
    EXPECT_EQ(static_cast<int>(remaining.size()), static_cast<int>(kStressChunkSize))
        << "Pool leaked nodes after exhaustion/refill rounds";
    returnAll(pool, remaining);
}

// ---------------------------------------------------------------------------
// Stress 3: constructor / destructor balance under concurrency.
// ---------------------------------------------------------------------------

TEST(LockFreeObjectPoolStress, ConstructorDestructorBalance)
{
    GuardedWidget::resetCounters();

    constexpr int kThreads = 8;
    constexpr int kIters = 5'000;

    StressPool pool(1);

    auto worker = [&](int tid)
    {
        for (int i = 0; i < kIters; ++i)
        {
            GuardedWidget* w = pool.tryAcquire(tid);
            if (!w)
            {
                continue;
            }
            pool.release(w);
        }
    };

    std::vector<std::thread> threads;
    threads.reserve(kThreads);
    for (int t = 0; t < kThreads; ++t)
    {
        threads.emplace_back(worker, t);
    }
    for (auto& t : threads)
    {
        t.join();
    }

    auto remaining = drainAll(pool);
    returnAll(pool, remaining);

    EXPECT_EQ(GuardedWidget::constructions_.load(), GuardedWidget::destructions_.load())
        << "Construction/destruction count mismatch — leaked or double-freed object";
}

// ---------------------------------------------------------------------------
// Stress 4: valid concurrent acquire/release must not corrupt pool state.
//
//   The old variant released an unowned pointer and expected eInvalidParam.
//   release() now asserts ownership, so this test focuses purely on verifying
//   that heavy concurrent valid traffic leaves the pool fully intact.
// ---------------------------------------------------------------------------

TEST(LockFreeObjectPoolStress, ConcurrentReleaseIsHarmless)
{
    constexpr int kThreads = 6;
    constexpr int kIters = 3'000;

    StressPool pool(1);

    auto worker = [&](int tid)
    {
        for (int i = 0; i < kIters; ++i)
        {
            GuardedWidget* w = pool.tryAcquire(tid);
            if (!w)
            {
                continue;
            }
            pool.release(w);
        }
    };

    std::vector<std::thread> threads;
    threads.reserve(kThreads);
    for (int t = 0; t < kThreads; ++t)
    {
        threads.emplace_back(worker, t);
    }
    for (auto& t : threads)
    {
        t.join();
    }

    auto remaining = drainAll(pool);
    EXPECT_EQ(static_cast<int>(remaining.size()), static_cast<int>(kStressChunkSize))
        << "Pool lost nodes during concurrent load";
    returnAll(pool, remaining);
}

// ---------------------------------------------------------------------------
// Stress 5: thundering-herd release — all threads release simultaneously.
//   Maximises contention on the pushNode CAS retry path.
// ---------------------------------------------------------------------------

TEST(LockFreeObjectPoolStress, ThunderingHerdRelease)
{
    constexpr int kThreads = static_cast<int>(kStressChunkSize);

    StressPool pool(1);

    auto slots = drainAll(pool);
    ASSERT_EQ(static_cast<int>(slots.size()), kThreads);

    std::latch ready(kThreads + 1);

    std::vector<std::thread> threads;
    threads.reserve(kThreads);
    for (int t = 0; t < kThreads; ++t)
    {
        threads.emplace_back(
            [&, t]()
            {
                ready.arrive_and_wait();
                pool.release(slots[t]);
            });
    }

    ready.arrive_and_wait();
    for (auto& t : threads)
    {
        t.join();
    }

    auto remaining = drainAll(pool);
    EXPECT_EQ(static_cast<int>(remaining.size()), kThreads)
        << "Thundering-herd release lost " << (kThreads - remaining.size()) << " node(s)";
    returnAll(pool, remaining);
}

// ---------------------------------------------------------------------------
// Stress 6: sustained mixed load over a wall-clock duration.
//   Increase kDuration for overnight soak runs.
// ---------------------------------------------------------------------------

TEST(LockFreeObjectPoolStress, SustainedMixedLoad)
{
    using namespace std::chrono_literals;

    constexpr int kThreads = 12;
    const auto    kDuration = 2s;

    StressPool pool(1);

    auto           all = drainAll(pool);
    GuardedWidget* base =
        all.empty() ? nullptr : *std::min_element(all.begin(), all.end());
    returnAll(pool, all);

    std::array<std::atomic<int>, kStressChunkSize> inUse{};
    for (auto& f : inUse)
    {
        f.store(0, std::memory_order_relaxed);
    }

    std::atomic<bool> stop{ false };
    std::atomic<long> totalAcquired{ 0 };
    std::atomic<bool> corruptionDetected{ false };

    auto worker = [&](int tid)
    {
        while (!stop.load(std::memory_order_relaxed))
        {
            GuardedWidget* w = pool.tryAcquire(tid);
            if (!w)
            {
                continue;
            }

            ++totalAcquired;

            if (base)
            {
                const std::ptrdiff_t idx = w - base;
                if (idx >= 0 && static_cast<usize>(idx) < kStressChunkSize)
                {
                    if (inUse[idx].exchange(1, std::memory_order_acq_rel) != 0)
                    {
                        corruptionDetected.store(true, std::memory_order_relaxed);
                    }

                    w->stamp(tid);
                    if (!w->verifyStamp(tid))
                    {
                        corruptionDetected.store(true, std::memory_order_relaxed);
                    }

                    inUse[idx].store(0, std::memory_order_release);
                }
            }

            pool.release(w);
        }
    };

    std::vector<std::thread> threads;
    threads.reserve(kThreads);
    for (int t = 0; t < kThreads; ++t)
    {
        threads.emplace_back(worker, t);
    }

    std::this_thread::sleep_for(kDuration);
    stop.store(true, std::memory_order_relaxed);
    for (auto& t : threads)
    {
        t.join();
    }

    EXPECT_FALSE(corruptionDetected.load())
        << "Data corruption detected during sustained load";
    EXPECT_GT(totalAcquired.load(), 0L);

    auto remaining = drainAll(pool);
    EXPECT_EQ(static_cast<int>(remaining.size()), static_cast<int>(kStressChunkSize))
        << "Pool leaked " << (kStressChunkSize - remaining.size())
        << " node(s) under sustained load";
    returnAll(pool, remaining);
}

// ═══════════════════════════════════════════════════════════════════════════
// Chunk allocation tests
// ═══════════════════════════════════════════════════════════════════════════

// ---------------------------------------------------------------------------
// Capacity boundary — single chunk (maxChunksCount == 1)
// ---------------------------------------------------------------------------

TEST(LockFreeObjectPoolChunks, SingleChunkExhaustsAtCapacity)
{
    SmallPool pool(1);

    std::vector<Widget*> held;
    for (int i = 0; i < 4; ++i)
    {
        Widget* w = pool.tryAcquire(i);
        ASSERT_NE(w, nullptr) << "Expected slot " << i << " to be available";
        held.push_back(w);
    }

    EXPECT_EQ(pool.tryAcquire(99), nullptr)
        << "Pool must not grow beyond a single chunk when maxChunksCount=1";

    for (Widget* w : held)
    {
        pool.release(w);
    }
}

// ---------------------------------------------------------------------------
// Second chunk allocated on exhaustion of the first
// ---------------------------------------------------------------------------

TEST(LockFreeObjectPoolChunks, SecondChunkAllocatedWhenFirstExhausted)
{
    SmallPool pool(2);

    std::vector<Widget*> firstChunk;
    for (int i = 0; i < 4; ++i)
    {
        Widget* w = pool.tryAcquire(i);
        ASSERT_NE(w, nullptr);
        firstChunk.push_back(w);
    }

    Widget* fromSecondChunk = pool.tryAcquire(100);
    EXPECT_NE(fromSecondChunk, nullptr)
        << "Pool failed to allocate a second chunk on exhaustion";

    if (fromSecondChunk)
    {
        pool.release(fromSecondChunk);
    }
    for (Widget* w : firstChunk)
    {
        pool.release(w);
    }
}

// ---------------------------------------------------------------------------
// Total capacity spans all chunks
// ---------------------------------------------------------------------------

TEST(LockFreeObjectPoolChunks, TotalCapacitySpansAllChunks)
{
    constexpr int kChunks = 3;
    constexpr int kSlots = 4; // SmallPool: 4 slots per chunk
    constexpr int kTotal = kChunks * kSlots;

    SmallPool pool(kChunks);

    std::vector<Widget*> held;
    held.reserve(kTotal);

    for (int i = 0; i < kTotal; ++i)
    {
        Widget* w = pool.tryAcquire(i);
        ASSERT_NE(w, nullptr) << "Expected slot " << i << " across " << kChunks
                              << " chunks; got nullptr";
        held.push_back(w);
    }

    EXPECT_EQ(pool.tryAcquire(999), nullptr)
        << "Pool must not exceed maxChunksCount * NumOfObjectsPerChunk slots";

    for (Widget* w : held)
    {
        pool.release(w);
    }
}

// ---------------------------------------------------------------------------
// Chunk count is not exceeded under concurrent pressure
// ---------------------------------------------------------------------------

TEST(LockFreeObjectPoolChunks, MaxChunksCountRespectedUnderConcurrentExhaustion)
{
    constexpr int kChunks = 2;
    constexpr int kCapacity = kChunks * 4; // SmallPool: 4 slots per chunk
    constexpr int kThreads = 8;
    constexpr int kIters = 2'000;

    SmallPool        pool(kChunks);
    std::atomic<int> peakHeld{ 0 };
    std::atomic<int> currentHeld{ 0 };

    auto worker = [&](int tid)
    {
        for (int i = 0; i < kIters; ++i)
        {
            Widget* w = pool.tryAcquire(tid);
            if (!w)
            {
                continue;
            }

            int now = currentHeld.fetch_add(1, std::memory_order_acq_rel) + 1;

            int expected = peakHeld.load(std::memory_order_relaxed);
            while (
                now > expected &&
                !peakHeld.compare_exchange_weak(expected, now, std::memory_order_relaxed))
            {
            }

            currentHeld.fetch_sub(1, std::memory_order_acq_rel);
            pool.release(w);
        }
    };

    std::vector<std::thread> threads;
    threads.reserve(kThreads);
    for (int t = 0; t < kThreads; ++t)
    {
        threads.emplace_back(worker, t);
    }
    for (auto& t : threads)
    {
        t.join();
    }

    EXPECT_LE(peakHeld.load(), kCapacity)
        << "More slots were simultaneously held than the pool's total capacity";
}

// ---------------------------------------------------------------------------
// Released nodes are reused before a new chunk is allocated
// ---------------------------------------------------------------------------

TEST(LockFreeObjectPoolChunks, FreedNodeReusedBeforeNewChunkAllocated)
{
    SmallPool pool(1);

    Widget* slots[4];
    for (int i = 0; i < 4; ++i)
    {
        slots[i] = pool.tryAcquire(i);
    }

    pool.release(slots[0]);
    slots[0] = nullptr;

    Widget* recycled = pool.tryAcquire(42);
    EXPECT_NE(recycled, nullptr)
        << "Freed node was not recycled — pool appears to have lost it";

    if (recycled)
    {
        pool.release(recycled);
    }
    for (int i = 1; i < 4; ++i)
    {
        if (slots[i])
        {
            pool.release(slots[i]);
        }
    }
}

// ---------------------------------------------------------------------------
// Constructor / destructor balance across dynamically allocated chunks
// ---------------------------------------------------------------------------

TEST(LockFreeObjectPoolChunks, ConstructorDestructorBalanceAcrossChunks)
{
    Widget::resetCounters();

    constexpr int kChunks = 3;
    constexpr int kTotal = kChunks * 8; // TrackedPool: 8 slots per chunk

    TrackedPool pool(kChunks);

    std::vector<TrackedWidget*> held;
    held.reserve(kTotal);
    for (int i = 0; i < kTotal; ++i)
    {
        TrackedWidget* w = pool.tryAcquire(i);
        if (w)
        {
            held.push_back(w);
        }
    }

    for (TrackedWidget* w : held)
    {
        pool.release(w);
    }

    EXPECT_EQ(Widget::constructions_.load(), Widget::destructions_.load())
        << "Construction/destruction mismatch across dynamically allocated chunks";
}

// ---------------------------------------------------------------------------
// All dynamically allocated chunks are freed on pool destruction
// ---------------------------------------------------------------------------

TEST(LockFreeObjectPoolChunks, AllDynamicChunksFreedOnDestruction)
{
    constexpr int kChunks = 4;

    const i64 before = mm::getMemSize();
    {
        SmallPool pool(kChunks);

        std::vector<Widget*> held;
        held.reserve(kChunks * 4);
        for (int i = 0; i < kChunks * 4; ++i)
        {
            Widget* w = pool.tryAcquire(i);
            if (w)
            {
                held.push_back(w);
            }
        }
        for (Widget* w : held)
        {
            pool.release(w);
        }

        // ~SmallPool() runs here — all dynamic chunks must be mm::free'd.
    }
    const i64 after = mm::getMemSize();

    EXPECT_EQ(before, after) << "Memory leak: " << (after - before)
                             << " bytes not freed after pool destruction";
}

// ---------------------------------------------------------------------------
// Stress: concurrent chunk allocation under thundering-herd exhaustion
// ---------------------------------------------------------------------------

TEST(LockFreeObjectPoolChunks, ConcurrentChunkAllocationUnderLoad)
{
    constexpr int kChunks = 4; // up to 4*16 = 64 live objects
    constexpr int kThreads = 8;
    constexpr int kIters = 5'000;
    constexpr int kCapacity = kChunks * 16; // GrowStress: 16 slots per chunk

    GrowStress pool(kChunks);

    GuardedWidget::resetCounters();

    std::atomic<bool> corruptionDetected{ false };
    std::latch        startLine(kThreads + 1);

    auto worker = [&](int tid)
    {
        startLine.arrive_and_wait();

        for (int i = 0; i < kIters; ++i)
        {
            GuardedWidget* w = pool.tryAcquire(tid);
            if (!w)
            {
                continue;
            }

            w->stamp(tid);
            if (!w->verifyStamp(tid))
            {
                corruptionDetected.store(true, std::memory_order_relaxed);
            }

            pool.release(w);
        }
    };

    std::vector<std::thread> threads;
    threads.reserve(kThreads);
    for (int t = 0; t < kThreads; ++t)
    {
        threads.emplace_back(worker, t);
    }

    startLine.arrive_and_wait();
    for (auto& t : threads)
    {
        t.join();
    }

    EXPECT_FALSE(corruptionDetected.load())
        << "Data corruption detected during concurrent chunk allocation";

    auto remaining = drainAll(pool);
    EXPECT_EQ(static_cast<int>(remaining.size()), kCapacity)
        << "Pool lost nodes across dynamically allocated chunks";
    returnAll(pool, remaining);
}

// ---------------------------------------------------------------------------
// No memory leak after concurrent multi-chunk usage
// ---------------------------------------------------------------------------

TEST(LockFreeObjectPoolChunks, NoMemoryLeakAfterConcurrentMultiChunkUsage)
{
    constexpr int kChunks = 4;
    constexpr int kThreads = 8;
    constexpr int kIters = 3'000;

    const i64 before = mm::getMemSize();
    {
        GrowStress pool(kChunks);

        auto worker = [&](int tid)
        {
            for (int i = 0; i < kIters; ++i)
            {
                GuardedWidget* w = pool.tryAcquire(tid);
                if (!w)
                {
                    continue;
                }
                pool.release(w);
            }
        };

        std::vector<std::thread> threads;
        threads.reserve(kThreads);
        for (int t = 0; t < kThreads; ++t)
        {
            threads.emplace_back(worker, t);
        }
        for (auto& t : threads)
        {
            t.join();
        }
    }
    const i64 after = mm::getMemSize();

    EXPECT_EQ(before, after) << "Memory leak of " << (after - before)
                             << " bytes after concurrent multi-chunk pool was destroyed";
}

// ═══════════════════════════════════════════════════════════════════════════
// index() tests
//
// The global index formula is:
//   chunkLocalIndex + PerChunkObjectCount * chunk->index
//
// base_ always has chunk->index == 0, so its N slots map to [0, N).
// Dynamic chunks receive chunk->index values beyond 1, so gaps may exist
// between the base chunk's range and a dynamic chunk's range. Tests therefore
// check *uniqueness* and *stability* rather than exact dense packing.
// ═══════════════════════════════════════════════════════════════════════════

// ---------------------------------------------------------------------------
// Single chunk: all indices fall in [0, N) and are pairwise distinct.
// ---------------------------------------------------------------------------

TEST(LockFreeObjectPoolIndex, SingleChunkIndicesAreUniqueAndInRange)
{
    constexpr usize kN = 4; // SmallPool slots per chunk
    SmallPool       pool(1);

    Widget*         slots[kN]{};
    std::set<usize> seen;

    for (usize i = 0; i < kN; ++i)
    {
        slots[i] = pool.tryAcquire(static_cast<int>(i));
        ASSERT_NE(slots[i], nullptr);

        usize idx = pool.index(slots[i]);
        EXPECT_LT(idx, kN) << "Index " << idx << " out of [0, " << kN << ") for slot "
                           << i;
        EXPECT_TRUE(seen.insert(idx).second)
            << "Duplicate index " << idx << " at slot " << i;
    }

    for (auto* w : slots)
    {
        pool.release(w);
    }
}

// ---------------------------------------------------------------------------
// Index is stable across acquire → release → re-acquire of the same slot.
//   Because release pushes a node back to the free list and tryAcquire pops
//   it, the recycled pointer must carry the same global index.
// ---------------------------------------------------------------------------

TEST(LockFreeObjectPoolIndex, IndexIsStableAfterRecycle)
{
    SmallPool pool(1);

    Widget* w = pool.tryAcquire(1);
    ASSERT_NE(w, nullptr);

    const usize firstIndex = pool.index(w);

    // Fill the rest of the pool so the freed slot is the only one available.
    Widget* others[3];
    for (int i = 0; i < 3; ++i)
    {
        others[i] = pool.tryAcquire(i + 10);
    }

    pool.release(w);
    w = pool.tryAcquire(99); // must recycle the same node
    ASSERT_NE(w, nullptr);

    EXPECT_EQ(pool.index(w), firstIndex)
        << "Index changed after the slot was released and re-acquired";

    pool.release(w);
    for (auto* o : others)
    {
        pool.release(o);
    }
}

// ---------------------------------------------------------------------------
// Multi-chunk: indices are unique across the full pool capacity.
//   With two chunks the index ranges are non-overlapping by construction
//   (chunk->index differs), so this verifies no aliasing across chunk
//   boundaries.
// ---------------------------------------------------------------------------

TEST(LockFreeObjectPoolIndex, MultiChunkIndicesAreGloballyUnique)
{
    constexpr int kChunks = 3;
    constexpr int kTotal = kChunks * 4; // SmallPool: 4 slots per chunk

    SmallPool pool(kChunks);

    std::vector<Widget*> held;
    held.reserve(kTotal);
    std::set<usize> seen;

    for (int i = 0; i < kTotal; ++i)
    {
        Widget* w = pool.tryAcquire(i);
        ASSERT_NE(w, nullptr) << "Expected slot " << i << " to be available";

        usize idx = pool.index(w);
        EXPECT_TRUE(seen.insert(idx).second)
            << "Duplicate index " << idx << " returned for slot " << i
            << " (possible chunk aliasing)";

        held.push_back(w);
    }

    for (Widget* w : held)
    {
        pool.release(w);
    }
}

// ---------------------------------------------------------------------------
// Concurrent uniqueness: at every instant no two threads hold an object
//   with the same index.  An atomic flag array (one entry per possible index
//   position) acts as the concurrency-safe "in use" sentinel; a CAS from 0→1
//   on entry and 1→0 on exit detects any aliasing.
// ---------------------------------------------------------------------------

TEST(LockFreeObjectPoolIndex, ConcurrentIndexUniqueness)
{
    // Use a generous upper bound for the flag array.  The base chunk always
    // occupies indices [0, kN), and dynamic chunks land at multiples of kN
    // determined by their chunk->index.  With kChunks == 2 and kN == 16 the
    // highest possible index is PerChunkObjectCount * (currentChunkCount+1) - 1
    // which with the current assignment (chunk->index = currentChunkCount + 1)
    // tops out at 16 * 3 - 1 == 47.  We size the array conservatively.
    constexpr int kChunks = 2;
    constexpr int kN = 16;                        // GrowStress slots per chunk
    constexpr int kFlagSize = kN * (kChunks + 2); // safe upper bound
    constexpr int kThreads = 8;
    constexpr int kIters = 10'000;

    GrowStress pool(kChunks);

    std::array<std::atomic<int>, kFlagSize> inUse{};
    for (auto& f : inUse)
    {
        f.store(0, std::memory_order_relaxed);
    }

    std::atomic<bool> aliasDetected{ false };
    std::latch        startLine(kThreads + 1);

    auto worker = [&](int tid)
    {
        startLine.arrive_and_wait();

        for (int i = 0; i < kIters; ++i)
        {
            GuardedWidget* w = pool.tryAcquire(tid);
            if (!w)
            {
                continue;
            }

            const usize idx = pool.index(w);
            ASSERT_LT(idx, static_cast<usize>(kFlagSize))
                << "index() returned " << idx << " which exceeds the flag array bound";

            // Mark slot as in-use; any prior occupant means a double-acquire.
            if (inUse[idx].exchange(1, std::memory_order_acq_rel) != 0)
            {
                aliasDetected.store(true, std::memory_order_relaxed);
            }

            // Brief critical section: stamp and verify as in the stress suite.
            w->stamp(tid);
            EXPECT_TRUE(w->verifyStamp(tid)) << "Stamp mismatch at index " << idx
                                             << " (another thread is sharing our slot)";

            inUse[idx].store(0, std::memory_order_release);
            pool.release(w);
        }
    };

    std::vector<std::thread> threads;
    threads.reserve(kThreads);
    for (int t = 0; t < kThreads; ++t)
    {
        threads.emplace_back(worker, t);
    }

    startLine.arrive_and_wait();
    for (auto& t : threads)
    {
        t.join();
    }

    EXPECT_FALSE(aliasDetected.load())
        << "Two threads simultaneously held objects with the same index";
}

} // namespace mk::test
