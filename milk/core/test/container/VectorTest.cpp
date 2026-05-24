#include <gtest/gtest.h>
#include <core/container/IVector.h>

// ─────────────────────────────────────────────
// Helpers
// ─────────────────────────────────────────────

struct Trivial
{
    int  x = 0;
    bool operator==(const Trivial& o) const
    {
        return x == o.x;
    }
};

struct NonTrivial
{
    static int instances_;
    int        value;

    NonTrivial(): value(0)
    {
        ++instances_;
    }
    explicit NonTrivial(int v): value(v)
    {
        ++instances_;
    }
    NonTrivial(const NonTrivial& o): value(o.value)
    {
        ++instances_;
    }
    NonTrivial(NonTrivial&& o) noexcept: value(o.value)
    {
        ++instances_;
        o.value = -1;
    }
    ~NonTrivial()
    {
        --instances_;
    }

    bool operator==(const NonTrivial& o) const
    {
        return value == o.value;
    }
};

int NonTrivial::instances_ = 0;

struct NonTrivialTest: ::testing::Test
{
    void SetUp() override
    {
        NonTrivial::instances_ = 0;
    }
    void TearDown() override
    {
        EXPECT_EQ(NonTrivial::instances_, 0);
    }
};

// ─────────────────────────────────────────────
// Construction
// ─────────────────────────────────────────────
namespace mk
{

TEST(VectorConstruction, DefaultConstructedIsEmpty)
{
    Vector<int> v;
    EXPECT_TRUE(v.empty());
    EXPECT_EQ(v.size(), 0U);
}

TEST(VectorConstruction, InitializerList)
{
    Vector<int> v = { 1, 2, 3 };
    ASSERT_EQ(v.size(), 3U);
    EXPECT_EQ(v[0], 1);
    EXPECT_EQ(v[1], 2);
    EXPECT_EQ(v[2], 3);
}

TEST(VectorConstruction, CopyConstructor)
{
    Vector<int> a = { 10, 20, 30 };
    Vector<int> b = a;
    ASSERT_EQ(b.size(), 3U);
    EXPECT_EQ(b[0], 10);
    EXPECT_EQ(b[2], 30);

    // Ensure deep copy — mutating a doesn't affect b
    a[0] = 99;
    EXPECT_EQ(b[0], 10);
}

TEST(VectorConstruction, MoveConstructor)
{
    Vector<int> a = { 1, 2, 3 };
    Vector<int> b = std::move(a);
    ASSERT_EQ(b.size(), 3U);
    EXPECT_EQ(b[0], 1);
    EXPECT_TRUE(a.empty());
}

TEST_F(NonTrivialTest, DestructorCalledOnScopeExit)
{
    {
        Vector<NonTrivial> v;
        v.emplace(1);
        v.emplace(2);
        EXPECT_EQ(NonTrivial::instances_, 2);
    }
    EXPECT_EQ(NonTrivial::instances_, 0);
}

// ─────────────────────────────────────────────
// Assignment
// ─────────────────────────────────────────────

TEST(VectorAssignment, CopyAssignment)
{
    Vector<int> a = { 1, 2, 3 };
    Vector<int> b;
    b = a;
    ASSERT_EQ(b.size(), 3U);
    EXPECT_EQ(b[1], 2);
    a[1] = 99;
    EXPECT_EQ(b[1], 2); // deep copy
}

TEST(VectorAssignment, MoveAssignment)
{
    Vector<int> a = { 7, 8, 9 };
    Vector<int> b;
    b = std::move(a);
    ASSERT_EQ(b.size(), 3U);
    EXPECT_EQ(b[0], 7);
    EXPECT_TRUE(a.empty());
}

TEST(VectorAssignment, InitializerListAssignment)
{
    Vector<int> v = { 1, 2, 3 };
    v = { 10, 20 };
    ASSERT_EQ(v.size(), 2U);
    EXPECT_EQ(v[0], 10);
    EXPECT_EQ(v[1], 20);
}

TEST_F(NonTrivialTest, CopyAssignmentDestroysOldElems)
{
    Vector<NonTrivial> a;
    a.emplace(1);
    a.emplace(2);
    {
        Vector<NonTrivial> b;
        b.emplace(9);
        b = a;                                // old elem in b should be destroyed
        EXPECT_EQ(NonTrivial::instances_, 4); // 2 in a, 2 in b
    }
    EXPECT_EQ(NonTrivial::instances_, 2); // only a remains
}

// ─────────────────────────────────────────────
// push / emplace / pop
// ─────────────────────────────────────────────

TEST(VectorMutation, PushCopy)
{
    Vector<int> v;
    int         x = 42;
    v.push(x);
    ASSERT_EQ(v.size(), 1U);
    EXPECT_EQ(v[0], 42);
}

TEST(VectorMutation, PushMove)
{
    Vector<std::string> v;
    std::string         s = "hello";
    v.push(std::move(s));
    ASSERT_EQ(v.size(), 1U);
    EXPECT_EQ(v[0], "hello");
    EXPECT_TRUE(s.empty()); // moved-from
}

TEST(VectorMutation, EmplaceInPlace)
{
    Vector<std::pair<int, int>> v;
    v.emplace(3, 4);
    ASSERT_EQ(v.size(), 1U);
    EXPECT_EQ(v[0].first, 3);
    EXPECT_EQ(v[0].second, 4);
}

TEST(VectorMutation, Pop)
{
    Vector<int> v = { 1, 2, 3 };
    int         val = v.pop();
    EXPECT_EQ(val, 3);
    EXPECT_EQ(v.size(), 2U);
}

TEST_F(NonTrivialTest, PopDestructsCorrectly)
{
    Vector<NonTrivial> v;
    v.emplace(1);
    v.emplace(2);
    EXPECT_EQ(NonTrivial::instances_, 2);
    auto popped = v.pop();
    EXPECT_EQ(popped.value, 2);
    EXPECT_EQ(NonTrivial::instances_, 2); // popped still alive
}

TEST(VectorMutation, PushTriggersGrowth)
{
    Vector<int> v;
    for (int i = 0; i < 1024; ++i)
    {
        v.push(i);
    }
    ASSERT_EQ(v.size(), 1024U);
    for (int i = 0; i < 1024; ++i)
    {
        EXPECT_EQ(v[i], i);
    }
}

// ─────────────────────────────────────────────
// reserve / resize / clear
// ─────────────────────────────────────────────

TEST(VectorCapacity, ReserveDoesNotChangeSize)
{
    Vector<int> v;
    v.reserve(100);
    EXPECT_EQ(v.size(), 0U);
    EXPECT_GE(v.capacity(), 100U);
}

TEST(VectorCapacity, ResizeGrowsWithDefaultValues)
{
    Vector<int> v;
    v.resize(5);
    ASSERT_EQ(v.size(), 5U);
    for (usize i = 0; i < 5; ++i)
    {
        EXPECT_EQ(v[i], 0);
    }
}

TEST(VectorCapacity, ResizeShrinks)
{
    Vector<int> v = { 1, 2, 3, 4, 5 };
    v.resize(2);
    ASSERT_EQ(v.size(), 2U);
    EXPECT_EQ(v[0], 1);
    EXPECT_EQ(v[1], 2);
}

TEST_F(NonTrivialTest, ResizeShrinkDestructs)
{
    Vector<NonTrivial> v;
    v.emplace(1);
    v.emplace(2);
    v.emplace(3);
    EXPECT_EQ(NonTrivial::instances_, 3);
    v.resize(1);
    EXPECT_EQ(NonTrivial::instances_, 1);
}

TEST(VectorCapacity, ClearEmptiesVector)
{
    Vector<int> v = { 1, 2, 3 };
    v.clear();
    EXPECT_TRUE(v.empty());
    EXPECT_EQ(v.size(), 0U);
}

TEST_F(NonTrivialTest, ClearDestructsAll)
{
    Vector<NonTrivial> v;
    v.emplace(1);
    v.emplace(2);
    EXPECT_EQ(NonTrivial::instances_, 2);
    v.clear();
    EXPECT_EQ(NonTrivial::instances_, 0);
    EXPECT_TRUE(v.empty());
}

// ─────────────────────────────────────────────
// Iterators & data access
// ─────────────────────────────────────────────

TEST(VectorIteration, RangeForLoop)
{
    Vector<int> v = { 10, 20, 30 };
    int         sum = 0;
    for (auto x : v)
    {
        sum += x;
    }
    EXPECT_EQ(sum, 60);
}

TEST(VectorIteration, BeginEndDistance)
{
    Vector<int> v = { 1, 2, 3, 4 };
    EXPECT_EQ(v.end() - v.begin(), 4);
}

TEST(VectorIteration, DataPointerMatchesBegin)
{
    Vector<int> v = { 5, 6, 7 };
    EXPECT_EQ(v.data(), v.begin());
}

// ─────────────────────────────────────────────
// FixedVector (stack storage)
// ─────────────────────────────────────────────

TEST(FixedVector, BasicPushAndAccess)
{
    FixedVector<int, 8> v;
    v.push(1);
    v.push(2);
    ASSERT_EQ(v.size(), 2U);
    EXPECT_EQ(v[0], 1);
    EXPECT_EQ(v[1], 2);
}

TEST(FixedVector, InitializerList)
{
    FixedVector<int, 4> v = { 10, 20, 30 };
    ASSERT_EQ(v.size(), 3U);
    EXPECT_EQ(v[2], 30);
}

// ─────────────────────────────────────────────
// StackVector (inline + heap fallback)
// ─────────────────────────────────────────────

TEST(StackVector, PushWithinInlineCapacity)
{
    StackVector<int, 4> v;
    v.push(1);
    v.push(2);
    ASSERT_EQ(v.size(), 2U);
    EXPECT_EQ(v[0], 1);
}

TEST(StackVector, GrowsBeyondInlineCapacity)
{
    StackVector<int, 2> v;
    for (int i = 0; i < 8; ++i)
    {
        v.push(i);
    }
    ASSERT_EQ(v.size(), 8U);
    for (int i = 0; i < 8; ++i)
    {
        EXPECT_EQ(v[i], i);
    }
}

// ─────────────────────────────────────────────
// Cross-storage construction / assignment
// ─────────────────────────────────────────────

TEST(CrossStorage, HeapFromStackCopy)
{
    FixedVector<int, 8> fixed = { 1, 2, 3 };
    Vector<int>         heap(fixed);
    ASSERT_EQ(heap.size(), 3U);
    EXPECT_EQ(heap[0], 1);
    EXPECT_EQ(heap[2], 3);
}

TEST(CrossStorage, HeapAssignFromStack)
{
    FixedVector<int, 8> fixed = { 4, 5, 6 };
    Vector<int>         heap;
    heap = fixed;
    ASSERT_EQ(heap.size(), 3U);
    EXPECT_EQ(heap[1], 5);
}

// ─────────────────────────────────────────────
// VectorView
// ─────────────────────────────────────────────

TEST(VectorView, ConstructFromVector)
{
    Vector<int>     v = { 1, 2, 3 };
    VectorView<int> view(v);
    ASSERT_EQ(view.size(), 3U);
    EXPECT_EQ(view[0], 1);
    EXPECT_EQ(view[2], 3);
}

TEST(VectorView, ConstructFromCArray)
{
    int             arr[] = { 10, 20, 30, 40 };
    VectorView<int> view(arr, 4);
    ASSERT_EQ(view.size(), 4U);
    EXPECT_EQ(view[3], 40);
}

TEST(VectorView, DefaultIsEmpty)
{
    VectorView<int> view;
    EXPECT_TRUE(view.empty());
    EXPECT_EQ(view.size(), 0U);
}

TEST(VectorView, MutationThroughView)
{
    Vector<int>     v = { 1, 2, 3 };
    VectorView<int> view(v);
    view[1] = 99;
    EXPECT_EQ(v[1], 99);
}

TEST(VectorView, IterationMatchesSource)
{
    Vector<int>     v = { 5, 10, 15 };
    VectorView<int> view(v);
    int             sum = 0;
    for (auto x : view)
    {
        sum += x;
    }
    EXPECT_EQ(sum, 30);
}

TEST(VectorView, DataPointerMatchesVector)
{
    Vector<int>     v = { 1, 2 };
    VectorView<int> view(v);
    EXPECT_EQ(view.cdata(), v.data());
}
} // namespace mk
