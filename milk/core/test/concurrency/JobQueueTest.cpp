#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <thread>
#include <vector>
#include <algorithm>

#include <core/concurrency/jobsystem/IJobQueue.h>

namespace mk::cc
{

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

// Thin RAII wrapper that gives each test a stable set of JobInstance objects
// addressable by index so we can verify ordering without heap allocation per
// test.
template<std::size_t N>
struct InstancePool
{
    details::JobInstance items[N] = {};

    details::JobInstance* at(std::size_t i)
    {
        return &items[i];
    }
};

// ===========================================================================
// 1. Empty-queue behaviour
// ===========================================================================

TEST(JobQueueEmpty, PopReturnsFalse)
{
    JobQueue<64>          q;
    details::JobInstance* out = nullptr;
    EXPECT_FALSE(q.tryDequeue(out));
    EXPECT_EQ(out, nullptr); // must not be written on failure
}

TEST(JobQueueEmpty, StealReturnsFalse)
{
    JobQueue<64>          q;
    details::JobInstance* out = nullptr;
    EXPECT_FALSE(q.trySteal(out));
    EXPECT_EQ(out, nullptr);
}

// ===========================================================================
// 2. Single-element round-trips
// ===========================================================================

TEST(JobQueueSingle, PushThenPop)
{
    JobQueue<64>    q;
    InstancePool<1> pool;

    ASSERT_TRUE(q.tryEnqueue(pool.at(0)));

    details::JobInstance* out = nullptr;
    ASSERT_TRUE(q.tryDequeue(out));
    EXPECT_EQ(out, pool.at(0));
}

TEST(JobQueueSingle, PushThenSteal)
{
    JobQueue<64>    q;
    InstancePool<1> pool;

    ASSERT_TRUE(q.tryEnqueue(pool.at(0)));

    details::JobInstance* out = nullptr;
    ASSERT_TRUE(q.trySteal(out));
    EXPECT_EQ(out, pool.at(0));
}

TEST(JobQueueSingle, PopEmptiesQueue)
{
    JobQueue<64>    q;
    InstancePool<1> pool;

    q.tryEnqueue(pool.at(0));

    details::JobInstance* out = nullptr;
    q.tryDequeue(out);

    EXPECT_FALSE(q.tryDequeue(out)); // must be empty now
    EXPECT_FALSE(q.trySteal(out));
}

TEST(JobQueueSingle, StealEmptiesQueue)
{
    JobQueue<64>    q;
    InstancePool<1> pool;

    q.tryEnqueue(pool.at(0));

    details::JobInstance* out = nullptr;
    q.trySteal(out);

    EXPECT_FALSE(q.tryDequeue(out));
    EXPECT_FALSE(q.trySteal(out));
}

// ===========================================================================
// 3. Ordering guarantees
// ===========================================================================

TEST(JobQueueOrder, PopIsLIFO)
{
    // push 0,1,2 → pop should yield 2,1,0
    JobQueue<64>    q;
    InstancePool<3> pool;

    for (int i = 0; i < 3; ++i)
    {
        q.tryEnqueue(pool.at(i));
    }

    details::JobInstance* out = nullptr;
    ASSERT_TRUE(q.tryDequeue(out));
    EXPECT_EQ(out, pool.at(2));
    ASSERT_TRUE(q.tryDequeue(out));
    EXPECT_EQ(out, pool.at(1));
    ASSERT_TRUE(q.tryDequeue(out));
    EXPECT_EQ(out, pool.at(0));
    EXPECT_FALSE(q.tryDequeue(out));
}

TEST(JobQueueOrder, StealIsFIFO)
{
    // push 0,1,2 → steal should yield 0,1,2
    JobQueue<64>    q;
    InstancePool<3> pool;

    for (int i = 0; i < 3; ++i)
    {
        q.tryEnqueue(pool.at(i));
    }

    details::JobInstance* out = nullptr;
    ASSERT_TRUE(q.trySteal(out));
    EXPECT_EQ(out, pool.at(0));
    ASSERT_TRUE(q.trySteal(out));
    EXPECT_EQ(out, pool.at(1));
    ASSERT_TRUE(q.trySteal(out));
    EXPECT_EQ(out, pool.at(2));
    EXPECT_FALSE(q.trySteal(out));
}

// ===========================================================================
// 4. Capacity / boundary conditions
// ===========================================================================

TEST(JobQueueCapacity, PushUpToMaxSucceeds)
{
    constexpr u32      kCap = 8;
    JobQueue<kCap>     q;
    InstancePool<kCap> pool;

    for (u32 i = 0; i < kCap; ++i)
    {
        EXPECT_TRUE(q.tryEnqueue(pool.at(i))) << "push failed at slot " << i;
    }
}

TEST(JobQueueCapacity, PushBeyondMaxReturnsFalse)
{
    constexpr u32          kCap = 4;
    JobQueue<kCap>         q;
    InstancePool<kCap + 1> pool;

    for (u32 i = 0; i < kCap; ++i)
    {
        q.tryEnqueue(pool.at(i));
    }

    EXPECT_FALSE(q.tryEnqueue(pool.at(kCap)));
}

TEST(JobQueueCapacity, ReusableAfterDrain)
{
    // Fill → drain via pop → fill again — ensures internal indices wrap correctly.
    constexpr u32      kCap = 8;
    JobQueue<kCap>     q;
    InstancePool<kCap> pool;

    for (u32 i = 0; i < kCap; ++i)
    {
        q.tryEnqueue(pool.at(i));
    }

    details::JobInstance* out = nullptr;
    for (u32 i = 0; i < kCap; ++i)
    {
        q.tryDequeue(out);
    }

    for (u32 i = 0; i < kCap; ++i)
    {
        EXPECT_TRUE(q.tryEnqueue(pool.at(i))) << "refill failed at slot " << i;
    }
}

TEST(JobQueueCapacity, ReusableAfterDrainBySteal)
{
    constexpr u32      kCap = 8;
    JobQueue<kCap>     q;
    InstancePool<kCap> pool;

    for (u32 i = 0; i < kCap; ++i)
    {
        q.tryEnqueue(pool.at(i));
    }

    details::JobInstance* out = nullptr;
    for (u32 i = 0; i < kCap; ++i)
    {
        q.trySteal(out);
    }

    for (u32 i = 0; i < kCap; ++i)
    {
        EXPECT_TRUE(q.tryEnqueue(pool.at(i)))
            << "refill after steal failed at slot " << i;
    }
}

// ===========================================================================
// 5. Interleaved single-threaded push/pop/steal
// ===========================================================================

TEST(JobQueueInterleaved, PushPopAlternating)
{
    JobQueue<64>    q;
    InstancePool<4> pool;

    details::JobInstance* out = nullptr;

    q.tryEnqueue(pool.at(0));
    ASSERT_TRUE(q.tryDequeue(out));
    EXPECT_EQ(out, pool.at(0));

    q.tryEnqueue(pool.at(1));
    q.tryEnqueue(pool.at(2));
    ASSERT_TRUE(q.tryDequeue(out));
    EXPECT_EQ(out, pool.at(2)); // LIFO

    q.tryEnqueue(pool.at(3));
    ASSERT_TRUE(q.tryDequeue(out));
    EXPECT_EQ(out, pool.at(3));
    ASSERT_TRUE(q.tryDequeue(out));
    EXPECT_EQ(out, pool.at(1)); // oldest remaining
}

TEST(JobQueueInterleaved, PushStealAlternating)
{
    JobQueue<64>    q;
    InstancePool<4> pool;

    details::JobInstance* out = nullptr;

    q.tryEnqueue(pool.at(0));
    ASSERT_TRUE(q.trySteal(out));
    EXPECT_EQ(out, pool.at(0));

    q.tryEnqueue(pool.at(1));
    q.tryEnqueue(pool.at(2));
    ASSERT_TRUE(q.trySteal(out));
    EXPECT_EQ(out, pool.at(1)); // FIFO
    ASSERT_TRUE(q.trySteal(out));
    EXPECT_EQ(out, pool.at(2));
}

TEST(JobQueueInterleaved, MixedPopAndStealDontDuplicate)
{
    // Alternately pop and steal from a 4-item queue.
    // Each item must be delivered exactly once.
    JobQueue<64>    q;
    InstancePool<4> pool;

    for (int i = 0; i < 4; ++i)
    {
        q.tryEnqueue(pool.at(i));
    }

    std::vector<details::JobInstance*> received;
    details::JobInstance*              out = nullptr;

    // steal from head, pop from tail, interleaved
    if (q.trySteal(out))
    {
        received.push_back(out);
    }
    if (q.tryDequeue(out))
    {
        received.push_back(out);
    }
    if (q.trySteal(out))
    {
        received.push_back(out);
    }
    if (q.tryDequeue(out))
    {
        received.push_back(out);
    }

    ASSERT_EQ(received.size(), 4U);

    // No duplicates
    std::sort(received.begin(), received.end());
    auto unique_end = std::unique(received.begin(), received.end());
    EXPECT_EQ(unique_end, received.end()) << "duplicate item delivered";
}

// ===========================================================================
// 6. Concurrent steal (multiple thieves, one owner doing no work)
// ===========================================================================

TEST(JobQueueConcurrent, MultipleThievesNoLoss)
{
    // Push N items, then let T thief threads race to steal all of them.
    // Every item must be stolen exactly once; no item lost, none duplicated.
    constexpr int kN = 128;
    constexpr int kT = 4;

    JobQueue<256>    q;
    InstancePool<kN> pool;

    for (int i = 0; i < kN; ++i)
    {
        q.tryEnqueue(pool.at(i));
    }

    std::vector<std::atomic<int>> stolen_count(kN);
    for (auto& c : stolen_count)
    {
        c.store(0);
    }

    std::vector<std::thread> thieves;
    thieves.reserve(kT);

    for (int t = 0; t < kT; ++t)
    {
        thieves.emplace_back(
            [&]()
            {
                details::JobInstance* out = nullptr;
                while (q.steal(out))
                {
                    // Identify which pool item this is
                    auto idx = static_cast<std::size_t>(out - pool.items);
                    ASSERT_LT(idx, static_cast<std::size_t>(kN));
                    stolen_count[idx].fetch_add(1, std::memory_order_relaxed);
                    out = nullptr;
                }
            });
    }

    for (auto& th : thieves)
    {
        th.join();
    }

    for (int i = 0; i < kN; ++i)
    {
        EXPECT_EQ(stolen_count[i].load(), 1)
            << "item " << i << " stolen " << stolen_count[i] << " times";
    }
}

// ===========================================================================
// 7. Concurrent owner + thieves (the classic Chase-Lev scenario)
// ===========================================================================

TEST(JobQueueConcurrent, OwnerPopsWhileThievesSteals)
{
    // Owner pushes items one-by-one and pops some itself; thieves steal
    // concurrently.  The invariant is: every item delivered exactly once,
    // no item lost, no item duplicated.
    constexpr int kN = 1;
    constexpr int kT = 3;

    JobQueue<512>    q;
    InstancePool<kN> pool;

    std::atomic<int> owner_done{ 0 };

    // Track deliveries with an atomic per slot
    std::vector<std::atomic<int>> delivery(kN);
    for (auto& d : delivery)
    {
        d.store(0);
    }

    auto record = [&](details::JobInstance* p)
    {
        auto idx = static_cast<std::size_t>(p - pool.items);
        ASSERT_LT(idx, static_cast<std::size_t>(kN));
        delivery[idx].fetch_add(1, std::memory_order_relaxed);
    };

    // Thief threads
    std::vector<std::thread> thieves;
    thieves.reserve(kT);
    for (int t = 0; t < kT; ++t)
    {
        thieves.emplace_back(
            [&]()
            {
                details::JobInstance* out = nullptr;
                while (!owner_done.load(std::memory_order_acquire) || q.steal(out))
                {
                    if (out)
                    {
                        record(out);
                        out = nullptr;
                    }
                }
                // Drain after owner signals done
                while (q.steal(out))
                {
                    record(out);
                    out = nullptr;
                }
            });
    }

    // Owner thread: push all items, occasionally pop half of what it pushed
    for (int i = 0; i < kN; ++i)
    {
        q.tryEnqueue(pool.at(i));
        // Pop every other item (simulates owner consuming its own work)
        if (i % 2 == 0)
        {
            details::JobInstance* out = nullptr;
            if (q.pop(out))
            {
                record(out);
            }
        }
    }

    // Drain remaining via pop
    {
        details::JobInstance* out = nullptr;
        while (q.pop(out))
        {
            record(out);
            out = nullptr;
        }
    }

    owner_done.store(1, std::memory_order_release);
    for (auto& th : thieves)
    {
        th.join();
    }

    for (int i = 0; i < kN; ++i)
    {
        EXPECT_EQ(delivery[i].load(), 1)
            << "item " << i << " delivered " << delivery[i] << " times";
    }
}

// ===========================================================================
// 8. The "last element" race — pop vs steal when size == 1
// ===========================================================================
//
//  Chase-Lev is most subtle when head == tail-1 (one element remains).
//  Both pop() and steal() may attempt to take it simultaneously; exactly
//  one must succeed and one must return false or retry.
//
TEST(JobQueueConcurrent, LastElementRacePopVsSteal)
{
    constexpr int kRounds = 10'000;

    InstancePool<1>  pool;
    std::atomic<int> pop_wins{ 0 };
    std::atomic<int> steal_wins{ 0 };

    for (int r = 0; r < kRounds; ++r)
    {
        JobQueue<4> q;
        q.tryEnqueue(pool.at(0));

        std::atomic<bool> thief_ready{ false };
        std::atomic<bool> owner_go{ false };

        std::thread thief(
            [&]()
            {
                thief_ready.store(true, std::memory_order_release);
                while (!owner_go.load(std::memory_order_acquire))
                {
                    std::this_thread::yield();
                }

                details::JobInstance* out = nullptr;
                if (q.trySteal(out))
                {
                    steal_wins.fetch_add(1, std::memory_order_relaxed);
                }
            });

        // Synchronise both sides to maximise contention
        while (!thief_ready.load(std::memory_order_acquire))
        {
            std::this_thread::yield();
        }
        owner_go.store(true, std::memory_order_release);

        details::JobInstance* out = nullptr;
        if (q.tryDequeue(out))
        {
            pop_wins.fetch_add(1, std::memory_order_relaxed);
        }

        thief.join();

        // Exactly one of pop or steal must have won each round
        ASSERT_EQ(pop_wins.load() + steal_wins.load(), r + 1)
            << "item lost or duplicated at round " << r;
    }
}

// ===========================================================================
// 9. High-frequency push/pop/steal stress test
// ===========================================================================

TEST(JobQueueStress, ProducerConsumerBalance)
{
    // Owner continuously pushes and pops in a loop while T thieves steal.
    // After a fixed number of iterations both sides stop; the queue must be
    // left in a consistent (possibly non-empty) state with no missing items.
    constexpr int kIters = 50'000;
    constexpr int kT = 4;

    JobQueue<128>   q;
    InstancePool<1> pool; // reuse same pointer — we only care about count

    std::atomic<int>  total_produced{ 0 };
    std::atomic<int>  total_consumed{ 0 };
    std::atomic<bool> done{ false };

    std::vector<std::thread> thieves;
    thieves.reserve(kT);
    for (int t = 0; t < kT; ++t)
    {
        thieves.emplace_back(
            [&]()
            {
                details::JobInstance* out = nullptr;
                while (!done.load(std::memory_order_acquire))
                {
                    if (q.steal(out))
                    {
                        total_consumed.fetch_add(1, std::memory_order_relaxed);
                        out = nullptr;
                    }
                }
                // Final drain
                while (q.steal(out))
                {
                    total_consumed.fetch_add(1, std::memory_order_relaxed);
                    out = nullptr;
                }
            });
    }

    // Owner
    for (int i = 0; i < kIters; ++i)
    {
        if (q.tryEnqueue(pool.at(0)))
        {
            total_produced.fetch_add(1, std::memory_order_relaxed);
        }

        // Occasionally pop rather than leaving work for thieves
        if (i % 4 == 0)
        {
            details::JobInstance* out = nullptr;
            if (q.pop(out))
            {
                total_consumed.fetch_add(1, std::memory_order_relaxed);
            }
        }
    }

    // Owner drains whatever remains
    {
        details::JobInstance* out = nullptr;
        while (q.pop(out))
        {
            total_consumed.fetch_add(1, std::memory_order_relaxed);
            out = nullptr;
        }
    }

    done.store(true, std::memory_order_release);
    for (auto& th : thieves)
    {
        th.join();
    }

    EXPECT_EQ(total_consumed.load(), total_produced.load())
        << "produced=" << total_produced.load() << " consumed=" << total_consumed.load();
}

} // namespace mk::cc
