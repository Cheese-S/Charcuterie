#include <test/ISimpleTest.h>
#include <core/IMemory.h>
#include <core/ISharedPtr.h>
#include <core/IUniquePtr.h>
#include <new>

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

TEST(UniquePtr, CustomDeleter)
{
    class CustomDeleter
    {
    public:
        void operator()(int* ptr) const
        {
            *ptr = 5;
        }
    };

    int a = 0;
    EXPECT_EQ(a, 0);
    {
        UniquePtr<int, CustomDeleter> ptr(&a);
    }
    EXPECT_EQ(a, 5);
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

// ═════════════════════════════════════════════
// mm allocator — alloc / realloc / free / new
// ═════════════════════════════════════════════

TEST(MmAllocator, AllocDefaultAlignment)
{
    void* p = mm::alloc(100);
    ASSERT_NE(p, nullptr);
    EXPECT_EQ(reinterpret_cast<uptr>(p) % mm::details::kDefaultAlignment, 0U);
    mm::free(p);
}

TEST(MmAllocator, AllocReturnsAligned)
{
    const usize alignments[] = { 16, 64, 256 };
    const usize sizes[] = { 1, 16, 100, 1024 };

    for (usize alignment : alignments)
    {
        for (usize size : sizes)
        {
            void* p = mm::alloc(size, alignment);
            ASSERT_NE(p, nullptr);
            EXPECT_EQ(reinterpret_cast<uptr>(p) % alignment, 0U);
            mm::free(p);
        }
    }
}

TEST(MmAllocator, ReallocGrowPreservesData)
{
    constexpr usize kInitial = 4;
    constexpr usize kGrown = 8;

    int* p = static_cast<int*>(mm::alloc(kInitial * sizeof(int)));
    ASSERT_NE(p, nullptr);
    for (int i = 0; i < static_cast<int>(kInitial); ++i)
    {
        p[i] = i;
    }

    int* q = static_cast<int*>(mm::realloc(p, kGrown * sizeof(int)));
    ASSERT_NE(q, nullptr);
    EXPECT_EQ(reinterpret_cast<uptr>(q) % mm::details::kDefaultAlignment, 0U);
    for (int i = 0; i < static_cast<int>(kInitial); ++i)
    {
        EXPECT_EQ(q[i], i);
    }
    for (int i = static_cast<int>(kInitial); i < static_cast<int>(kGrown); ++i)
    {
        q[i] = i;
        EXPECT_EQ(q[i], i);
    }
    mm::free(q);
}

TEST(MmAllocator, ReallocShrinkPreservesPrefix)
{
    constexpr usize kInitial = 8;
    constexpr usize kShrunk = 4;

    int* p = static_cast<int*>(mm::alloc(kInitial * sizeof(int)));
    ASSERT_NE(p, nullptr);
    for (int i = 0; i < static_cast<int>(kInitial); ++i)
    {
        p[i] = i;
    }

    int* q = static_cast<int*>(mm::realloc(p, kShrunk * sizeof(int)));
    ASSERT_NE(q, nullptr);
    EXPECT_EQ(reinterpret_cast<uptr>(q) % mm::details::kDefaultAlignment, 0U);
    for (int i = 0; i < static_cast<int>(kShrunk); ++i)
    {
        EXPECT_EQ(q[i], i);
    }
    mm::free(q);
}

TEST(MmAllocator, ReallocNullActsLikeAlloc)
{
    int* p = static_cast<int*>(mm::realloc(nullptr, 32 * sizeof(int)));
    ASSERT_NE(p, nullptr);
    EXPECT_EQ(reinterpret_cast<uptr>(p) % mm::details::kDefaultAlignment, 0U);
    p[0] = 42;
    EXPECT_EQ(p[0], 42);
    mm::free(p);
}

TEST(MmAllocator, ReallocReturnsAligned)
{
    const usize alignments[] = { 64, 256 };

    for (usize alignment : alignments)
    {
        void* p = mm::alloc(32, alignment);
        ASSERT_NE(p, nullptr);

        void* grown = mm::realloc(p, 128, alignment);
        ASSERT_NE(grown, nullptr);
        EXPECT_EQ(reinterpret_cast<uptr>(grown) % alignment, 0U);

        void* shrunk = mm::realloc(grown, 16, alignment);
        ASSERT_NE(shrunk, nullptr);
        EXPECT_EQ(reinterpret_cast<uptr>(shrunk) % alignment, 0U);

        mm::free(shrunk);
    }
}

TEST(MmAllocator, AllocFreeBalanced)
{
    const i64 before = mm::getMemSize();
    {
        void* p = mm::alloc(1024);
        ASSERT_NE(p, nullptr);
        mm::free(p);
    }
    EXPECT_EQ(mm::getMemSize(), before);
}

TEST(MmAllocator, ReallocBalanced)
{
    const i64 before = mm::getMemSize();
    {
        int* p = static_cast<int*>(mm::alloc(4 * sizeof(int)));
        ASSERT_NE(p, nullptr);
        int* grown = static_cast<int*>(mm::realloc(p, 8 * sizeof(int)));
        ASSERT_NE(grown, nullptr);
        int* shrunk = static_cast<int*>(mm::realloc(grown, 2 * sizeof(int)));
        ASSERT_NE(shrunk, nullptr);
        mm::free(shrunk);
    }
    EXPECT_EQ(mm::getMemSize(), before);
}

TEST(MmAllocator, GetGoodSize)
{
    EXPECT_EQ(mm::getGoodSize(0), 0U);

    const usize sizes[] = { 1, 17, 100, 4096 };
    for (usize size : sizes)
    {
        EXPECT_GE(mm::getGoodSize(size), size);
    }
}

TEST(MmAllocator, OperatorNewDeleteBalanced)
{
    const i64 before = mm::getMemSize();
    {
        int* p = new int(42);
        ASSERT_NE(p, nullptr);
        EXPECT_EQ(*p, 42);
        delete p;
    }
    EXPECT_EQ(mm::getMemSize(), before);
}

TEST(MmAllocator, OperatorNewArrayDeleteArrayBalanced)
{
    const i64 before = mm::getMemSize();
    {
        int* p = new int[16];
        ASSERT_NE(p, nullptr);
        for (int i = 0; i < 16; ++i)
        {
            p[i] = i;
        }
        EXPECT_EQ(p[15], 15);
        delete[] p;
    }
    EXPECT_EQ(mm::getMemSize(), before);
}

TEST(MmAllocator, OperatorNothrowNew)
{
    int* p = new (std::nothrow) int(7);
    ASSERT_NE(p, nullptr);
    EXPECT_EQ(*p, 7);
    delete p;

    int* arr = new (std::nothrow) int[4];
    ASSERT_NE(arr, nullptr);
    arr[3] = 3;
    EXPECT_EQ(arr[3], 3);
    delete[] arr;
}

TEST(MmAllocator, AlignedNewDelete)
{
    struct alignas(64) Aligned
    {
        int value;
    };

    Aligned* p = new Aligned();
    ASSERT_NE(p, nullptr);
    EXPECT_EQ(reinterpret_cast<uptr>(p) % 64, 0U);
    p->value = 42;
    EXPECT_EQ(p->value, 42);
    delete p;
}

} // namespace mk
MK_SIMPLE_MAIN()
