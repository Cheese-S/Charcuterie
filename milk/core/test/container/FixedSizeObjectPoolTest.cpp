#include <gtest/gtest.h>
#include <core/container/IFixedSizeObjectPool.h>

namespace mk
{

// ── Test fixture type ─────────────────────────────────────────────────────────

struct alignas(alignof(void*)) Obj
{
    void* pad = nullptr;
    int   value = 0;

    explicit Obj(int v = 0): value(v) {}
};

struct alignas(alignof(void*)) Tracked
{
    void* pad = nullptr;
    int*  counter = nullptr;
    int   id = 0;

    Tracked(int id, int* counter): counter(counter), id(id) {}
    ~Tracked()
    {
        if (counter)
        {
            ++(*counter);
        }
    }
};

// ── Acquire ───────────────────────────────────────────────────────────────────

TEST(FixedSizeObjectPool, AcquireReturnsNonNull)
{
    FixedSizeObjectPool<Obj, 4> pool;
    Obj*                        p = pool.tryAcquire(42);
    ASSERT_NE(p, nullptr);
    EXPECT_EQ(p->value, 42);
}

TEST(FixedSizeObjectPool, AcquireConstructsInPlace)
{
    FixedSizeObjectPool<Obj, 4> pool;
    Obj*                        a = pool.tryAcquire(1);
    Obj*                        b = pool.tryAcquire(2);
    Obj*                        c = pool.tryAcquire(3);
    EXPECT_EQ(a->value, 1);
    EXPECT_EQ(b->value, 2);
    EXPECT_EQ(c->value, 3);
}

TEST(FixedSizeObjectPool, AcquireReturnsDistinctPointers)
{
    FixedSizeObjectPool<Obj, 4> pool;
    Obj*                        a = pool.tryAcquire();
    Obj*                        b = pool.tryAcquire();
    Obj*                        c = pool.tryAcquire();
    EXPECT_NE(a, b);
    EXPECT_NE(b, c);
    EXPECT_NE(a, c);
}

TEST(FixedSizeObjectPool, AcquireReturnsNullWhenExhausted)
{
    FixedSizeObjectPool<Obj, 2> pool;
    pool.tryAcquire();
    pool.tryAcquire();
    EXPECT_EQ(pool.tryAcquire(), nullptr);
}

// ── isOwned ───────────────────────────────────────────────────────────────────

TEST(FixedSizeObjectPool, IsOwnedTrueForAcquiredPointer)
{
    FixedSizeObjectPool<Obj, 4> pool;
    Obj*                        p = pool.tryAcquire();
    EXPECT_TRUE(pool.isOwned(p));
}

TEST(FixedSizeObjectPool, IsOwnedFalseForExternalPointer)
{
    FixedSizeObjectPool<Obj, 4> pool;
    Obj                         external{};
    EXPECT_FALSE(pool.isOwned(&external));
}

TEST(FixedSizeObjectPool, IsOwnedFalseForNullptr)
{
    FixedSizeObjectPool<Obj, 4> pool;
    EXPECT_FALSE(pool.isOwned(nullptr));
}

TEST(FixedSizeObjectPool, IsOwnedFalseForMisalignedPointerInsideStorage)
{
    FixedSizeObjectPool<Obj, 4> pool;
    Obj*                        valid = pool.tryAcquire();
    uptr                        raw = reinterpret_cast<uptr>(valid) + 1;
    Obj*                        forged = reinterpret_cast<Obj*>(raw);
    EXPECT_FALSE(pool.isOwned(forged));
}

TEST(FixedSizeObjectPool, IsOwnedTrueForEverySlot)
{
    constexpr usize              kN = 8;
    FixedSizeObjectPool<Obj, kN> pool;
    Obj*                         ptrs[kN]{};
    for (usize i = 0; i < kN; i++)
    {
        ptrs[i] = pool.tryAcquire(static_cast<int>(i));
    }
    for (usize i = 0; i < kN; i++)
    {
        EXPECT_TRUE(pool.isOwned(ptrs[i])) << "slot " << i;
    }
}

TEST(FixedSizeObjectPool, ReleaseCallsDestructor)
{
    int                             dtorCount = 0;
    FixedSizeObjectPool<Tracked, 4> pool;
    Tracked*                        p = pool.tryAcquire(1, &dtorCount);
    ASSERT_NE(p, nullptr);
    EXPECT_EQ(pool.release(p), Result::eOk);
    EXPECT_EQ(dtorCount, 1);
}

TEST(FixedSizeObjectPool, ReleaseReturnsSlotToPool)
{
    FixedSizeObjectPool<Obj, 1> pool;
    Obj*                        p = pool.tryAcquire(7);
    ASSERT_NE(p, nullptr);
    EXPECT_EQ(pool.tryAcquire(), nullptr);

    EXPECT_EQ(pool.release(p), Result::eOk);
    Obj* recycled = pool.tryAcquire(8);
    ASSERT_NE(recycled, nullptr);
    EXPECT_EQ(recycled->value, 8);
}

TEST(FixedSizeObjectPool, ReleaseInvalidParamForExternalPointer)
{
    FixedSizeObjectPool<Obj, 4> pool;
    Obj                         external{};
    EXPECT_EQ(pool.release(&external), Result::eInvalidParam);
}

TEST(FixedSizeObjectPool, ReleaseInvalidParamForNullptr)
{
    FixedSizeObjectPool<Obj, 4> pool;
    EXPECT_EQ(pool.release(nullptr), Result::eInvalidParam);
}

TEST(FixedSizeObjectPool, ReleaseAndReacquireMultipleTimes)
{
    FixedSizeObjectPool<Obj, 2> pool;
    Obj*                        a = pool.tryAcquire(1);
    Obj*                        b = pool.tryAcquire(2);
    EXPECT_EQ(pool.tryAcquire(), nullptr);

    EXPECT_EQ(pool.release(a), Result::eOk);
    EXPECT_EQ(pool.release(b), Result::eOk);

    Obj* c = pool.tryAcquire(3);
    Obj* d = pool.tryAcquire(4);
    EXPECT_NE(c, nullptr);
    EXPECT_NE(d, nullptr);
    EXPECT_NE(c, d);
    EXPECT_EQ(pool.tryAcquire(), nullptr);
}

TEST(FixedSizeObjectPool, DestructorCountMatchesReleaseCount)
{
    int                             dtorCount = 0;
    FixedSizeObjectPool<Tracked, 4> pool;
    Tracked*                        a = pool.tryAcquire(1, &dtorCount);
    Tracked*                        b = pool.tryAcquire(2, &dtorCount);
    Tracked*                        c = pool.tryAcquire(3, &dtorCount);

    EXPECT_EQ(pool.release(a), Result::eOk);
    EXPECT_EQ(pool.release(b), Result::eOk);
    EXPECT_EQ(pool.release(c), Result::eOk);
    EXPECT_EQ(dtorCount, 3);
}

} // namespace mk
