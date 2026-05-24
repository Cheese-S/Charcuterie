#include <gtest/gtest.h>
#include <core/concurrency/jobsystem/IJobSystem.h>
#include <test/ITest.h>
#include <atomic>
#include <vector>
#include <mutex>

namespace mk::cc
{

class alignas(64) OrderTrackingJob: public IJob
{
public:
    struct Context
    {
        std::atomic<u32> counter{ 0U };
        std::vector<u32> order;
        std::mutex       mutex;

        void reset()
        {
            counter.store(0U);
            order.clear();
        }
    };

    OrderTrackingJob(Context& ctx, u32 id): ctx_(ctx), id_(id) {}

    void run() override
    {
        std::lock_guard<std::mutex> lock(ctx_.mutex);
        ctx_.order.push_back(id_);
        ctx_.counter.fetch_add(1U, std::memory_order_release);
    }

private:
    Context& ctx_;
    u32      id_;
};

class JobSystemTest: public MilkTest
{
protected:
    static constexpr JobPoolSizeConfig kPoolConfig = { .smallJobPoolSize = 20240,
                                                       .mediumJobPoolSize = 512,
                                                       .largeJobPoolSize = 256 };

    JobSystemTest(): job_system_({ .numForegroundThreads = 4, .numBackgorundThreads = 2 })
    {
    }

    JobSystem<kPoolConfig> job_system_;
};

class JobSystemDependencyTest: public JobSystemTest
{
protected:
    OrderTrackingJob::Context ctx_;

    void SetUp() override
    {
        JobSystemTest::SetUp();
        ctx_.reset();
    }
};

class alignas(64) RecursiveJob: public IJob
{
public:
    static constexpr u32 kBranchFactor = 4U;

    static constexpr u32 expectedCount(u32 depth)
    {
        u32 total = 0U;
        u32 level = 1U;
        for (u32 i = 0U; i <= depth; ++i)
        {
            total += level;
            level *= kBranchFactor;
        }
        return total;
    }

    RecursiveJob(IJobSystem& jobSystem, std::atomic<u32>& counter, u32 depth):
        jobSystem_(jobSystem), counter_(counter), depth_(depth)
    {
    }

    void run() override
    {
        counter_.fetch_add(1U, std::memory_order_release);
        if (depth_ == 0U)
        {
            return;
        }

        for (u32 i = 0U; i < kBranchFactor; ++i)
        {
            JobHandle child = allocJob<RecursiveJob>(jobSystem_,
                                                     JobPriority::eHigh,
                                                     jobSystem_,
                                                     counter_,
                                                     depth_ - 1U);
            jobSystem_.scheduleJob(child);
        }
    }

private:
    IJobSystem&       jobSystem_;
    std::atomic<u32>& counter_;
    u32               depth_;
};

TEST_F(JobSystemTest, ConstructionAndDestruction)
{
    // Actual call to constructor and destructor is in test fixture
    // Give time for worker threads to sleep
    std::this_thread::sleep_for(std::chrono::seconds(1));
}

TEST_F(JobSystemDependencyTest, ForegroundJobFinishes)
{
    JobHandle h = allocJob<OrderTrackingJob>(job_system_, JobPriority::eHigh, ctx_, 1U);
    job_system_.scheduleJob(h);

    // executeJobWhileWaiting must be false when called from a non-worker thread (like the
    // main thread) to avoid assertion failures in tryRunOneJob which expects a TLS worker
    // context.
    EXPECT_EQ(job_system_.waitForJob(h, false, util::kForever), Result::eOk);
    EXPECT_EQ(ctx_.counter.load(), 1U);
}

TEST_F(JobSystemDependencyTest, BackgroundJobFinishes)
{
    JobHandle h =
        allocJob<OrderTrackingJob>(job_system_, JobPriority::eBackgroundHigh, ctx_, 1U);
    job_system_.scheduleJob(h);

    EXPECT_EQ(job_system_.waitForJob(h, false, util::kForever), Result::eOk);
    EXPECT_EQ(ctx_.counter.load(), 1U);
}

TEST_F(JobSystemDependencyTest, SimpleDependency)
{
    JobHandle handleA =
        allocJob<OrderTrackingJob>(job_system_, JobPriority::eHigh, ctx_, 1U);
    JobHandle handleB =
        allocJob<OrderTrackingJob>(job_system_, JobPriority::eHigh, ctx_, 2U);

    job_system_.addJobDependency(handleA, handleB);

    // Both must be scheduled
    job_system_.scheduleJob(handleA);
    job_system_.scheduleJob(handleB);

    EXPECT_EQ(job_system_.waitForJob(handleA, false, util::kForever), Result::eOk);

    EXPECT_EQ(ctx_.counter.load(), 2U);
    ASSERT_EQ(ctx_.order.size(), 2U);
    EXPECT_EQ(ctx_.order[0], 2U); // B ran first
    EXPECT_EQ(ctx_.order[1], 1U); // A ran second
}

TEST_F(JobSystemDependencyTest, FinishedDependency)
{
    // Run and finish Job B first
    JobHandle handleB =
        allocJob<OrderTrackingJob>(job_system_, JobPriority::eHigh, ctx_, 2U);
    job_system_.scheduleJob(handleB);
    EXPECT_EQ(job_system_.waitForJob(handleB, false, util::kForever), Result::eOk);
    EXPECT_EQ(ctx_.counter.load(), 1U);

    // Now create A and make it depend on the already-finished B
    JobHandle handleA =
        allocJob<OrderTrackingJob>(job_system_, JobPriority::eHigh, ctx_, 1U);
    job_system_.addJobDependency(handleA, handleB);
    job_system_.scheduleJob(handleA);

    // This should not hang
    EXPECT_EQ(job_system_.waitForJob(handleA, false, util::kForever), Result::eOk);
    EXPECT_EQ(ctx_.counter.load(), 2U);
    EXPECT_EQ(ctx_.order.back(), 1U);
}

TEST_F(JobSystemDependencyTest, DiamondDependency)
{
    JobHandle handleA =
        allocJob<OrderTrackingJob>(job_system_, JobPriority::eHigh, ctx_, 1U);
    JobHandle handleB =
        allocJob<OrderTrackingJob>(job_system_, JobPriority::eHigh, ctx_, 2U);
    JobHandle handleC =
        allocJob<OrderTrackingJob>(job_system_, JobPriority::eHigh, ctx_, 3U);
    JobHandle handleD =
        allocJob<OrderTrackingJob>(job_system_, JobPriority::eHigh, ctx_, 4U);

    // B and C depend on D
    job_system_.addJobDependency(handleB, handleD);
    job_system_.addJobDependency(handleC, handleD);
    // A depends on B and C
    job_system_.addJobDependency(handleA, handleB);
    job_system_.addJobDependency(handleA, handleC);

    // Schedule all
    job_system_.scheduleJob(handleA);
    job_system_.scheduleJob(handleB);
    job_system_.scheduleJob(handleC);
    job_system_.scheduleJob(handleD);

    EXPECT_EQ(job_system_.waitForJob(handleA, false, util::kForever), Result::eOk);

    EXPECT_EQ(ctx_.counter.load(), 4U);
    ASSERT_EQ(ctx_.order.size(), 4U);
    EXPECT_EQ(ctx_.order[0], 4U); // D must be first
    EXPECT_EQ(ctx_.order[3], 1U); // A must be last
}

TEST_F(JobSystemDependencyTest, StressConcurrentAddDependencyAgainstLiveDependees)
{
    static constexpr u32 kIterCount = 1;
    static constexpr u32 kPairCount = 10000U;

    struct Pair
    {
        JobHandle                 dependee{};
        JobHandle                 depender{};
        OrderTrackingJob::Context ctx{};
    };

    for (u32 i = 0; i < kIterCount; i++)
    {
        std::vector<std::unique_ptr<Pair>> pairs(kPairCount);
        for (auto& p : pairs)
        {
            p = std::make_unique<Pair>();
            p->dependee =
                allocJob<OrderTrackingJob>(job_system_, JobPriority::eHigh, p->ctx, 2U);
            p->depender =
                allocJob<OrderTrackingJob>(job_system_, JobPriority::eHigh, p->ctx, 1U);

            MK_ASSERT(p->dependee.id != p->depender.id);

            job_system_.scheduleJob(p->dependee);
            job_system_.addJobDependency(p->depender, p->dependee);
            job_system_.scheduleJob(p->depender);
        }

        for (auto& p : pairs)
        {
            EXPECT_EQ(job_system_.waitForJob(p->depender, false, util::kForever),
                      Result::eOk);
            EXPECT_EQ(p->ctx.counter.load(), 2U)
                << "Both jobs must have run exactly once";
            EXPECT_EQ(p->ctx.order[0], 2U) << "Dependee must have run before depender";
            EXPECT_EQ(p->ctx.order[1], 1U) << "Depender must have run after dependee";
        }
    }
}

TEST_F(JobSystemDependencyTest, RecursiveJobSpawnsChildren)
{
    static constexpr u32 kDepth = 3U;
    static constexpr u32 kExpectedCount = RecursiveJob::expectedCount(kDepth);

    std::atomic<u32> counter{ 0U };

    JobHandle root = allocJob<RecursiveJob>(job_system_,
                                            JobPriority::eHigh,
                                            job_system_,
                                            counter,
                                            kDepth);
    job_system_.scheduleJob(root);

    constexpr auto kTimeout = std::chrono::seconds(5);
    const auto     deadline = std::chrono::steady_clock::now() + kTimeout;
    while (counter.load(std::memory_order_acquire) < kExpectedCount)
    {
        ASSERT_LT(std::chrono::steady_clock::now(), deadline)
            << "Timed out waiting for recursive jobs. Completed: " << counter.load()
            << " / " << kExpectedCount;
        std::this_thread::yield();
    }

    EXPECT_EQ(counter.load(), kExpectedCount);
}

TEST_F(JobSystemDependencyTest, CrossPriorityGroupDependency)
{
    JobHandle dependee =
        allocJob<OrderTrackingJob>(job_system_, JobPriority::eBackgroundHigh, ctx_, 2U);
    JobHandle depender =
        allocJob<OrderTrackingJob>(job_system_, JobPriority::eHigh, ctx_, 1U);

    job_system_.addJobDependency(depender, dependee);
    job_system_.scheduleJob(dependee);
    job_system_.scheduleJob(depender);

    EXPECT_EQ(job_system_.waitForJob(depender, false, util::kForever), Result::eOk);
    EXPECT_EQ(ctx_.counter.load(), 2U);
    ASSERT_EQ(ctx_.order.size(), 2U);
    EXPECT_EQ(ctx_.order[0], 2U); // Background dependee ran first
    EXPECT_EQ(ctx_.order[1], 1U); // Foreground depender ran second
}

TEST_F(JobSystemDependencyTest, LowPriorityJobsFinish)
{
    JobHandle foregroundLow =
        allocJob<OrderTrackingJob>(job_system_, JobPriority::eLow, ctx_, 1U);
    JobHandle backgroundLow =
        allocJob<OrderTrackingJob>(job_system_, JobPriority::eBackgroundLow, ctx_, 2U);

    job_system_.scheduleJob(foregroundLow);
    job_system_.scheduleJob(backgroundLow);

    EXPECT_EQ(job_system_.waitForJob(foregroundLow, false, util::kForever), Result::eOk);
    EXPECT_EQ(job_system_.waitForJob(backgroundLow, false, util::kForever), Result::eOk);
    EXPECT_EQ(ctx_.counter.load(), 2U);
}

TEST_F(JobSystemDependencyTest, GenericStress)
{
    static constexpr u32 kNumChains = 32U;
    static constexpr u32 kChainLength = 8U;
    static constexpr u32 kNumFanOut = 16U;
    static constexpr u32 kFanWidth = 8U;
    static constexpr u32 kNumRecursive = 8U;
    static constexpr u32 kRecursiveDepth = 3U;
    static constexpr u32 kIterCount = 100U;

    static constexpr JobPriority kPriorities[] = {
        JobPriority::eHigh,
        JobPriority::eLow,
        JobPriority::eBackgroundHigh,
        JobPriority::eBackgroundLow,
    };

    for (u32 i = 0; i < kIterCount; i++)
    {
        std::vector<JobHandle> leafHandles;
        std::mutex             leafMutex;

        // ── Linear chains of varying priority ────────────────────────────────────
        for (u32 c = 0U; c < kNumChains; ++c)
        {
            JobPriority prio = kPriorities[c % std::size(kPriorities)];

            JobHandle prev = allocJob<OrderTrackingJob>(job_system_, prio, ctx_, c);
            job_system_.scheduleJob(prev);

            for (u32 i = 1U; i < kChainLength; ++i)
            {
                JobHandle curr = allocJob<OrderTrackingJob>(job_system_, prio, ctx_, c);
                job_system_.addJobDependency(curr, prev);
                job_system_.scheduleJob(curr);
                prev = curr;
            }

            std::lock_guard lock(leafMutex);
            leafHandles.push_back(prev);
        }

        // ── Fan-out: one root unblocks many leaves, mixed cross-priority ─────────
        for (u32 f = 0U; f < kNumFanOut; ++f)
        {
            JobPriority rootPrio = kPriorities[f % std::size(kPriorities)];

            JobHandle root = allocJob<OrderTrackingJob>(job_system_, rootPrio, ctx_, f);
            job_system_.scheduleJob(root);

            for (u32 i = 0U; i < kFanWidth; ++i)
            {
                JobPriority leafPrio = kPriorities[(f + i + 1U) % std::size(kPriorities)];

                JobHandle leaf =
                    allocJob<OrderTrackingJob>(job_system_, leafPrio, ctx_, f);
                job_system_.addJobDependency(leaf, root);
                job_system_.scheduleJob(leaf);

                std::lock_guard lock(leafMutex);
                leafHandles.push_back(leaf);
            }
        }

        // ── Recursive jobs that spawn children from within run() ─────────────────
        std::atomic<u32> recursiveCounter{ 0U };
        for (u32 r = 0U; r < kNumRecursive; ++r)
        {
            JobHandle root = allocJob<RecursiveJob>(job_system_,
                                                    JobPriority::eHigh,
                                                    job_system_,
                                                    recursiveCounter,
                                                    kRecursiveDepth);
            job_system_.scheduleJob(root);
        }

        // ── Wait for all explicitly tracked leaf handles ──────────────────────────
        for (JobHandle leaf : leafHandles)
        {
            EXPECT_EQ(job_system_.waitForJob(leaf, false, util::kForever), Result::eOk);
        }

        // ── Poll until recursive subtrees also drain ──────────────────────────────
        static constexpr u32 kExpectedRecursive =
            kNumRecursive * RecursiveJob::expectedCount(kRecursiveDepth);

        const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(10);
        while (recursiveCounter.load(std::memory_order_acquire) < kExpectedRecursive)
        {
            ASSERT_LT(std::chrono::steady_clock::now(), deadline)
                << "Timed out waiting for recursive jobs. Completed: "
                << recursiveCounter.load() << " / " << kExpectedRecursive;
            std::this_thread::yield();
        }

        EXPECT_EQ(recursiveCounter.load(), kExpectedRecursive);
    }
}

} // namespace mk::cc
