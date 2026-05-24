#include <gtest/gtest.h>
#include <core/ISharedPtr.h>
#include <core/IUniquePtr.h>

namespace mk
{
// ─────────────────────────────────────────────
// Helpers
// ─────────────────────────────────────────────

struct Tracker
{
    static std::atomic<int> instances_;
    int                     value_;

    explicit Tracker(int value = 0): value_(value)
    {
        ++instances_;
    }
    Tracker(const Tracker& other): value_(other.value_)
    {
        ++instances_;
    }
    ~Tracker()
    {
        --instances_;
    }
};

std::atomic<int> Tracker::instances_ = 0;

struct TrackerTest: ::testing::Test
{
    void SetUp() override
    {
        Tracker::instances_ = 0;
    }
    void TearDown() override
    {
        EXPECT_EQ(Tracker::instances_, 0);
    }
};

struct TrackerMtTest: ::testing::Test
{
    void SetUp() override
    {
        Tracker::instances_ = 0;
    }
    void TearDown() override
    {
        EXPECT_EQ(Tracker::instances_.load(), 0);
    }
};

struct Base
{
    virtual ~Base() = default;
    virtual int id() const
    {
        return 0;
    }
};

struct Derived: Base
{
    int id() const override
    {
        return 1;
    }
};

// ═════════════════════════════════════════════
// UniquePtr<T>
// ═════════════════════════════════════════════

TEST_F(TrackerTest, UniqueDefaultIsNull)
{
    UniquePtr<Tracker> ptr;
    EXPECT_FALSE(ptr);
    EXPECT_EQ(ptr.get(), nullptr);
}

TEST_F(TrackerTest, UniquePtrOwnsObject)
{
    {
        UniquePtr<Tracker> ptr(new Tracker(42));
        ASSERT_TRUE(ptr);
        EXPECT_EQ(ptr->value_, 42);
        EXPECT_EQ((*ptr).value_, 42);
        EXPECT_EQ(Tracker::instances_, 1);
    }
    EXPECT_EQ(Tracker::instances_, 0);
}

TEST_F(TrackerTest, UniqueMakeUnique)
{
    {
        UniquePtr<Tracker> ptr = makeUnique<Tracker>(7);
        ASSERT_TRUE(ptr);
        EXPECT_EQ(ptr->value_, 7);
        EXPECT_EQ(Tracker::instances_, 1);
    }
    EXPECT_EQ(Tracker::instances_, 0);
}

TEST_F(TrackerTest, UniqueMoveConstructor)
{
    UniquePtr<Tracker> a(new Tracker(1));
    UniquePtr<Tracker> b = std::move(a);
    EXPECT_FALSE(a);
    ASSERT_TRUE(b);
    EXPECT_EQ(b->value_, 1);
    EXPECT_EQ(Tracker::instances_, 1);
}

TEST_F(TrackerTest, UniqueMoveAssignment)
{
    UniquePtr<Tracker> a(new Tracker(1));
    UniquePtr<Tracker> b(new Tracker(2));
    b = std::move(a);
    // Old object in b should be destroyed
    EXPECT_EQ(Tracker::instances_, 1);
    EXPECT_FALSE(a);
    ASSERT_TRUE(b);
    EXPECT_EQ(b->value_, 1);
}

TEST_F(TrackerTest, UniqueNullptrAssignment)
{
    UniquePtr<Tracker> ptr(new Tracker(1));
    EXPECT_EQ(Tracker::instances_, 1);
    ptr = nullptr;
    EXPECT_FALSE(ptr);
    EXPECT_EQ(Tracker::instances_, 0);
}

TEST_F(TrackerTest, UniqueRelease)
{
    UniquePtr<Tracker> ptr(new Tracker(3));
    Tracker*           raw = ptr.release();
    EXPECT_FALSE(ptr);
    ASSERT_NE(raw, nullptr);
    EXPECT_EQ(raw->value_, 3);
    EXPECT_EQ(Tracker::instances_, 1); // still alive, we own it now
    delete raw;
}

TEST_F(TrackerTest, UniqueReset)
{
    UniquePtr<Tracker> ptr(new Tracker(1));
    EXPECT_EQ(Tracker::instances_, 1);
    ptr.reset();
    EXPECT_FALSE(ptr);
    EXPECT_EQ(Tracker::instances_, 0);
}

TEST_F(TrackerTest, UniqueResetWithNewPtr)
{
    UniquePtr<Tracker> ptr(new Tracker(1));
    ptr.reset(new Tracker(2));
    EXPECT_EQ(Tracker::instances_, 1); // old one destroyed
    EXPECT_EQ(ptr->value_, 2);
}

TEST_F(TrackerTest, UniqueResetSamePointerIsNoop)
{
    Tracker*           raw = new Tracker(1);
    UniquePtr<Tracker> ptr(raw);
    ptr.reset(raw); // same pointer — should not double-free
    EXPECT_EQ(Tracker::instances_, 1);
    EXPECT_EQ(ptr->value_, 1);
}

// Derived → Base conversion
TEST(UniquePtrConversion, DerivedToBase)
{
    UniquePtr<Base> ptr = makeUnique<Derived>();
    ASSERT_TRUE(ptr);
    EXPECT_EQ(ptr->id(), 1);
}

TEST(UniquePtrConversion, MoveAssignDerivedToBase)
{
    UniquePtr<Derived> derived = makeUnique<Derived>();
    UniquePtr<Base>    base = std::move(derived);
    EXPECT_FALSE(derived);
    ASSERT_TRUE(base);
    EXPECT_EQ(base->id(), 1);
}

// ─────────────────────────────────────────────
// UniquePtr<T[]>
// ─────────────────────────────────────────────

TEST_F(TrackerTest, UniqueArrayMakeUnique)
{
    {
        UniquePtr<Tracker[]> arr = makeUnique<Tracker[]>(3);
        ASSERT_TRUE(arr);
        EXPECT_EQ(Tracker::instances_, 3);
        arr[0].value_ = 10;
        arr[1].value_ = 20;
        arr[2].value_ = 30;
        EXPECT_EQ(arr[0].value_, 10);
        EXPECT_EQ(arr[2].value_, 30);
    }
    EXPECT_EQ(Tracker::instances_, 0);
}

TEST_F(TrackerTest, UniqueArrayNullptrAssignment)
{
    UniquePtr<Tracker[]> arr = makeUnique<Tracker[]>(2);
    EXPECT_EQ(Tracker::instances_, 2);
    arr = nullptr;
    EXPECT_FALSE(arr);
    EXPECT_EQ(Tracker::instances_, 0);
}

TEST_F(TrackerTest, UniqueArrayRelease)
{
    UniquePtr<Tracker[]> arr = makeUnique<Tracker[]>(2);
    Tracker*             raw = arr.release();
    EXPECT_FALSE(arr);
    EXPECT_EQ(Tracker::instances_, 2);
    delete[] raw;
}

// ═════════════════════════════════════════════
// SharedPtr / makeShared
// ═════════════════════════════════════════════

TEST_F(TrackerTest, SharedDefaultIsNull)
{
    SharedPtr<Tracker> ptr;
    EXPECT_FALSE(ptr);
    EXPECT_EQ(ptr.get(), nullptr);
}

TEST_F(TrackerTest, SharedMakeShared)
{
    {
        SharedPtr<Tracker> ptr = makeShared<Tracker>(99);
        ASSERT_TRUE(ptr);
        EXPECT_EQ(ptr->value_, 99);
        EXPECT_EQ(ptr.getCount(), 1U);
        EXPECT_EQ(Tracker::instances_, 1);
    }
    EXPECT_EQ(Tracker::instances_, 0);
}

TEST_F(TrackerTest, SharedFromRawPtr)
{
    {
        SharedPtr<Tracker> ptr(new Tracker(5));
        ASSERT_TRUE(ptr);
        EXPECT_EQ(ptr->value_, 5);
        EXPECT_EQ(Tracker::instances_, 1);
    }
    EXPECT_EQ(Tracker::instances_, 0);
}

TEST_F(TrackerTest, SharedCopyIncreasesRefCount)
{
    SharedPtr<Tracker> a = makeShared<Tracker>(1);
    SharedPtr<Tracker> b = a;
    EXPECT_EQ(a.getCount(), 2U);
    EXPECT_EQ(b.getCount(), 2U);
    EXPECT_EQ(Tracker::instances_, 1);
}

TEST_F(TrackerTest, SharedCopyDestroyedReducesRefCount)
{
    SharedPtr<Tracker> a = makeShared<Tracker>(1);
    {
        SharedPtr<Tracker> b = a;
        EXPECT_EQ(a.getCount(), 2U);
    }
    EXPECT_EQ(a.getCount(), 1U);
    EXPECT_EQ(Tracker::instances_, 1);
}

TEST_F(TrackerTest, SharedObjectDestroyedWhenLastOwnerDies)
{
    SharedPtr<Tracker> a = makeShared<Tracker>(1);
    {
        SharedPtr<Tracker> b = a;
        a.reset();
        EXPECT_EQ(Tracker::instances_, 1); // b still alive
    }
    EXPECT_EQ(Tracker::instances_, 0);
}

TEST_F(TrackerTest, SharedMoveConstructor)
{
    SharedPtr<Tracker> a = makeShared<Tracker>(2);
    SharedPtr<Tracker> b = std::move(a);
    EXPECT_FALSE(a);
    ASSERT_TRUE(b);
    EXPECT_EQ(b.getCount(), 1U);
    EXPECT_EQ(Tracker::instances_, 1);
}

TEST_F(TrackerTest, SharedCopyAssignment)
{
    SharedPtr<Tracker> a = makeShared<Tracker>(1);
    SharedPtr<Tracker> b = makeShared<Tracker>(2);
    EXPECT_EQ(Tracker::instances_, 2);
    b = a;
    EXPECT_EQ(Tracker::instances_, 1); // old b object destroyed
    EXPECT_EQ(a.getCount(), 2U);
    EXPECT_EQ(b->value_, 1);
}

TEST_F(TrackerTest, SharedMoveAssignment)
{
    SharedPtr<Tracker> a = makeShared<Tracker>(1);
    SharedPtr<Tracker> b = makeShared<Tracker>(2);
    b = std::move(a);
    EXPECT_EQ(Tracker::instances_, 1);
    EXPECT_FALSE(a);
    EXPECT_EQ(b->value_, 1);
}

TEST_F(TrackerTest, SharedNullptrAssignment)
{
    SharedPtr<Tracker> ptr = makeShared<Tracker>(1);
    ptr = nullptr;
    EXPECT_FALSE(ptr);
    EXPECT_EQ(Tracker::instances_, 0);
}

TEST_F(TrackerTest, SharedReset)
{
    SharedPtr<Tracker> ptr = makeShared<Tracker>(1);
    ptr.reset();
    EXPECT_FALSE(ptr);
    EXPECT_EQ(Tracker::instances_, 0);
}

TEST_F(TrackerTest, SharedIsValid)
{
    SharedPtr<Tracker> ptr = makeShared<Tracker>(1);
    EXPECT_TRUE(ptr.isValid());
    ptr.reset();
    EXPECT_FALSE(ptr.isValid());
}

// Derived → Base conversion
TEST(SharedPtrConversion, DerivedToBase)
{
    SharedPtr<Base> ptr = makeShared<Derived>();
    ASSERT_TRUE(ptr);
    EXPECT_EQ(ptr->id(), 1);
}

TEST(SharedPtrConversion, CopyDerivedToBase)
{
    SharedPtr<Derived> derived = makeShared<Derived>();
    SharedPtr<Base>    base = derived;
    EXPECT_EQ(base->id(), 1);
    EXPECT_EQ(derived.getCount(), 2U);
}

// StSharedPtr (single-threaded)
TEST_F(TrackerTest, StSharedPtrBasicUsage)
{
    StSharedPtr<Tracker> a = makeShared<Tracker, false>(7);
    StSharedPtr<Tracker> b = a;
    EXPECT_EQ(a.getCount(), 2U);
    EXPECT_EQ(Tracker::instances_, 1);
}

// ═════════════════════════════════════════════
// WeakPtr
// ═════════════════════════════════════════════

TEST_F(TrackerTest, WeakDefaultIsNull)
{
    WeakPtr<Tracker> weak;
    EXPECT_FALSE(weak);
}

TEST_F(TrackerTest, WeakFromShared)
{
    SharedPtr<Tracker> shared = makeShared<Tracker>(1);
    WeakPtr<Tracker>   weak = shared;
    EXPECT_TRUE(weak);
    EXPECT_EQ(shared.getCount(), 1U); // weak ref does not increase strong count
}

TEST_F(TrackerTest, WeakLockReturnsValidShared)
{
    SharedPtr<Tracker> shared = makeShared<Tracker>(42);
    WeakPtr<Tracker>   weak = shared;
    SharedPtr<Tracker> locked = weak.lock();
    ASSERT_TRUE(locked);
    EXPECT_EQ(locked->value_, 42);
    EXPECT_EQ(shared.getCount(), 2U);
}

TEST_F(TrackerTest, WeakLockReturnsNullAfterSharedExpires)
{
    WeakPtr<Tracker> weak;
    {
        SharedPtr<Tracker> shared = makeShared<Tracker>(1);
        weak = shared;
        EXPECT_TRUE(weak);
    }
    // shared is gone — object should be destroyed
    EXPECT_EQ(Tracker::instances_, 0);
    SharedPtr<Tracker> locked = weak.lock();
    EXPECT_FALSE(locked);
}

TEST_F(TrackerTest, WeakCopyAssignment)
{
    SharedPtr<Tracker> shared = makeShared<Tracker>(1);
    WeakPtr<Tracker>   a = shared;
    WeakPtr<Tracker>   b;
    b = a;
    EXPECT_TRUE(b);
    SharedPtr<Tracker> locked = b.lock();
    ASSERT_TRUE(locked);
    EXPECT_EQ(locked->value_, 1);
}

TEST_F(TrackerTest, WeakMoveConstructor)
{
    SharedPtr<Tracker> shared = makeShared<Tracker>(1);
    WeakPtr<Tracker>   a = shared;
    WeakPtr<Tracker>   b = std::move(a);
    EXPECT_TRUE(b);
    SharedPtr<Tracker> locked = b.lock();
    ASSERT_TRUE(locked);
    EXPECT_EQ(locked->value_, 1);
}

TEST_F(TrackerTest, WeakMoveAssignment)
{
    SharedPtr<Tracker> shared = makeShared<Tracker>(1);
    WeakPtr<Tracker>   a = shared;
    WeakPtr<Tracker>   b;
    b = std::move(a);
    EXPECT_TRUE(b);
    SharedPtr<Tracker> locked = b.lock();
    ASSERT_TRUE(locked);
    EXPECT_EQ(locked->value_, 1);
}

TEST_F(TrackerTest, WeakAssignFromShared)
{
    SharedPtr<Tracker> shared = makeShared<Tracker>(5);
    WeakPtr<Tracker>   weak;
    weak = shared;
    SharedPtr<Tracker> locked = weak.lock();
    ASSERT_TRUE(locked);
    EXPECT_EQ(locked->value_, 5);
}

TEST_F(TrackerTest, WeakNullptrAssignment)
{
    SharedPtr<Tracker> shared = makeShared<Tracker>(1);
    WeakPtr<Tracker>   weak = shared;
    weak = nullptr;
    EXPECT_FALSE(weak);
    // shared still alive
    EXPECT_EQ(Tracker::instances_, 1);
}

TEST_F(TrackerTest, WeakReset)
{
    SharedPtr<Tracker> shared = makeShared<Tracker>(1);
    WeakPtr<Tracker>   weak = shared;
    weak.reset();
    EXPECT_FALSE(weak);
    EXPECT_EQ(Tracker::instances_, 1);
}

TEST_F(TrackerTest, WeakIsValidFalseAfterObjectDestroyed)
{
    WeakPtr<Tracker> weak;
    {
        SharedPtr<Tracker> shared = makeShared<Tracker>(1);
        weak = shared;
        EXPECT_TRUE(weak.isValid());
    }
    EXPECT_FALSE(weak.isValid());
}

// Multiple weak refs don't keep object alive
TEST_F(TrackerTest, MultipleWeaksDontKeepObjectAlive)
{
    WeakPtr<Tracker> a;
    WeakPtr<Tracker> b;
    {
        SharedPtr<Tracker> shared = makeShared<Tracker>(1);
        a = shared;
        b = shared;
    }
    EXPECT_EQ(Tracker::instances_, 0);
    EXPECT_FALSE(a.lock());
    EXPECT_FALSE(b.lock());
}

// Derived → Base conversion for WeakPtr
TEST(WeakPtrConversion, DerivedToBase)
{
    SharedPtr<Derived> derived = makeShared<Derived>();
    WeakPtr<Base>      weak = derived;
    SharedPtr<Base>    locked = weak.lock();
    ASSERT_TRUE(locked);
    EXPECT_EQ(locked->id(), 1);
}

// StWeakPtr (single-threaded)
TEST_F(TrackerTest, StWeakPtrBasicUsage)
{
    StSharedPtr<Tracker> shared = makeShared<Tracker, false>(3);
    StWeakPtr<Tracker>   weak = shared;
    StSharedPtr<Tracker> locked = weak.lock();
    ASSERT_TRUE(locked);
    EXPECT_EQ(locked->value_, 3);
}

static constexpr int kNumThreads = 16;
static constexpr int kOpsPerThread = 10000;

// ═════════════════════════════════════════════
// SharedPtr — concurrent copy / destroy
// ═════════════════════════════════════════════

// Each thread repeatedly copies and destroys the shared ptr.
// The object must be destroyed exactly once — after the last copy dies.
TEST_F(TrackerMtTest, ConcurrentCopyAndDestroy)
{
    SharedPtr<Tracker> shared = makeShared<Tracker>(1);

    std::vector<std::thread> threads;
    threads.reserve(kNumThreads);

    for (int i = 0; i < kNumThreads; ++i)
    {
        threads.emplace_back(
            [&shared]()
            {
                for (int j = 0; j < kOpsPerThread; ++j)
                {
                    SharedPtr<Tracker> local = shared; // addStrongRef
                    EXPECT_TRUE(local);
                    // local destroyed here — releaseStrongRef
                }
            });
    }

    for (auto& t : threads)
    {
        t.join();
    }

    // Only the original shared remains
    EXPECT_EQ(shared.getCount(), 1U);
    EXPECT_EQ(Tracker::instances_.load(), 1);
}

// ─────────────────────────────────────────────
// SharedPtr — concurrent reset races with copy
// ─────────────────────────────────────────────

// Half the threads copy, half reset their local copy.
// The original shared must survive until the end.
TEST_F(TrackerMtTest, ConcurrentCopyAndReset)
{
    SharedPtr<Tracker> shared = makeShared<Tracker>(2);

    std::vector<std::thread> threads;
    threads.reserve(kNumThreads);

    for (int i = 0; i < kNumThreads; ++i)
    {
        threads.emplace_back(
            [&shared, i]()
            {
                for (int j = 0; j < kOpsPerThread; ++j)
                {
                    SharedPtr<Tracker> local = shared;
                    if (i % 2 == 0)
                    {
                        local.reset();
                    }
                }
            });
    }

    for (auto& t : threads)
    {
        t.join();
    }

    EXPECT_TRUE(shared);
    EXPECT_EQ(shared.getCount(), 1U);
    EXPECT_EQ(Tracker::instances_.load(), 1);
}

// ─────────────────────────────────────────────
// SharedPtr — last owner dropped concurrently
// ─────────────────────────────────────────────

// Each thread gets one copy. All copies are dropped concurrently.
// Object must be destroyed exactly once.
TEST_F(TrackerMtTest, LastOwnerDroppedConcurrently)
{
    std::vector<SharedPtr<Tracker>> copies;
    copies.reserve(kNumThreads);

    {
        SharedPtr<Tracker> shared = makeShared<Tracker>(3);
        for (int i = 0; i < kNumThreads; ++i)
        {
            copies.push_back(shared);
        }
    } // original drops here

    EXPECT_EQ(Tracker::instances_.load(), 1); // copies still alive

    std::vector<std::thread> threads;
    threads.reserve(kNumThreads);

    for (int i = 0; i < kNumThreads; ++i)
    {
        threads.emplace_back([&copies, i]() { copies[i].reset(); });
    }

    for (auto& t : threads)
    {
        t.join();
    }

    EXPECT_EQ(Tracker::instances_.load(), 0);
}

// ═════════════════════════════════════════════
// WeakPtr — concurrent lock
// ═════════════════════════════════════════════

// All threads try to lock the same WeakPtr while the original shared is alive.
// Every lock must succeed and see a consistent value.
TEST_F(TrackerMtTest, ConcurrentWeakLockWhileAlive)
{
    SharedPtr<Tracker> shared = makeShared<Tracker>(99);
    WeakPtr<Tracker>   weak = shared;

    std::vector<std::thread> threads;
    threads.reserve(kNumThreads);

    for (int i = 0; i < kNumThreads; ++i)
    {
        threads.emplace_back(
            [&weak]()
            {
                for (int j = 0; j < kOpsPerThread; ++j)
                {
                    SharedPtr<Tracker> locked = weak.lock();
                    ASSERT_TRUE(locked);
                    EXPECT_EQ(locked->value_, 99);
                }
            });
    }

    for (auto& t : threads)
    {
        t.join();
    }

    EXPECT_EQ(Tracker::instances_.load(), 1);
}

// ─────────────────────────────────────────────
// WeakPtr — lock races with shared expiry
// ─────────────────────────────────────────────

// Half the threads try to lock; one thread lets the shared expire.
// A failed lock must return null — never a dangling pointer.
TEST_F(TrackerMtTest, ConcurrentWeakLockRacingExpiry)
{
    SharedPtr<Tracker> shared = makeShared<Tracker>(7);
    WeakPtr<Tracker>   weak = shared;

    std::vector<std::thread> threads;
    threads.reserve(kNumThreads);

    // Thread 0 destroys the shared after a short spin
    threads.emplace_back([&shared]() { shared.reset(); });

    for (int i = 1; i < kNumThreads; ++i)
    {
        threads.emplace_back(
            [&weak]()
            {
                for (int j = 0; j < kOpsPerThread; ++j)
                {
                    SharedPtr<Tracker> locked = weak.lock();
                    // Either succeeds with correct value, or fails cleanly — never UB
                    if (locked)
                    {
                        EXPECT_EQ(locked->value_, 7);
                    }
                }
            });
    }

    for (auto& t : threads)
    {
        t.join();
    }

    // Object destroyed once shared and all successful locks are gone
    EXPECT_EQ(Tracker::instances_.load(), 0);
}

// ─────────────────────────────────────────────
// WeakPtr — concurrent copy of weak
// ─────────────────────────────────────────────

TEST_F(TrackerMtTest, ConcurrentWeakCopy)
{
    SharedPtr<Tracker> shared = makeShared<Tracker>(5);
    WeakPtr<Tracker>   weak = shared;

    std::vector<std::thread> threads;
    threads.reserve(kNumThreads);

    for (int i = 0; i < kNumThreads; ++i)
    {
        threads.emplace_back(
            [&weak]()
            {
                for (int j = 0; j < kOpsPerThread; ++j)
                {
                    WeakPtr<Tracker>   localWeak = weak;
                    SharedPtr<Tracker> locked = localWeak.lock();
                    ASSERT_TRUE(locked);
                    EXPECT_EQ(locked->value_, 5);
                }
            });
    }

    for (auto& t : threads)
    {
        t.join();
    }

    EXPECT_EQ(Tracker::instances_.load(), 1);
}

// ─────────────────────────────────────────────
// RefCount integrity under high contention
// ─────────────────────────────────────────────

// Stress: threads repeatedly copy and destroy shared ptrs.
// Final ref count must be exactly 1.
TEST_F(TrackerMtTest, RefCountIntegrityUnderContention)
{
    SharedPtr<Tracker> shared = makeShared<Tracker>(0);

    std::vector<std::thread> threads;
    threads.reserve(kNumThreads);

    for (int i = 0; i < kNumThreads; ++i)
    {
        threads.emplace_back(
            [&shared]()
            {
                std::vector<SharedPtr<Tracker>> localCopies;
                localCopies.reserve(kOpsPerThread);
                for (int j = 0; j < kOpsPerThread; ++j)
                {
                    localCopies.push_back(shared);
                }
                // All local copies destroyed here
            });
    }

    for (auto& t : threads)
    {
        t.join();
    }

    EXPECT_EQ(shared.getCount(), 1U);
    EXPECT_EQ(Tracker::instances_.load(), 1);
}

} // namespace mk
