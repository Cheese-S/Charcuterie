#include <gtest/gtest.h>
#include <core/container/IString.h>

namespace mk
{

// ─────────────────────────────────────────────
// String Construction
// ─────────────────────────────────────────────

TEST(StringConstruction, DefaultIsEmpty)
{
    String s;
    EXPECT_TRUE(s.empty());
    EXPECT_EQ(s.size(), 0U);
    EXPECT_EQ(s.cstr()[0], '\0');
}

TEST(StringConstruction, FromCstr)
{
    String s("hello");
    EXPECT_EQ(s.size(), 5U);
    EXPECT_STREQ(s.cstr(), "hello");
}

TEST(StringConstruction, FromCstrWithLength)
{
    String s("hello world", 5);
    EXPECT_EQ(s.size(), 5U);
    EXPECT_STREQ(s.cstr(), "hello");
}

TEST(StringConstruction, FromStringView)
{
    StringView view("world");
    String     s(view);
    EXPECT_EQ(s.size(), 5U);
    EXPECT_STREQ(s.cstr(), "world");
}

TEST(StringConstruction, CopyConstructor)
{
    String a("hello");
    String b = a;
    EXPECT_STREQ(b.cstr(), "hello");
    // Deep copy — mutate a, b unchanged
    a[0] = 'X';
    EXPECT_STREQ(b.cstr(), "hello");
}

TEST(StringConstruction, MoveConstructor)
{
    String a("hello");
    String b = std::move(a);
    EXPECT_STREQ(b.cstr(), "hello");
}

// ─────────────────────────────────────────────
// String Assignment
// ─────────────────────────────────────────────

TEST(StringAssignment, CopyAssignment)
{
    String a("hello");
    String b("world");
    b = a;
    EXPECT_STREQ(b.cstr(), "hello");
    a[0] = 'X';
    EXPECT_STREQ(b.cstr(), "hello");
}

TEST(StringAssignment, MoveAssignment)
{
    String a("hello");
    String b("world");
    b = std::move(a);
    EXPECT_STREQ(b.cstr(), "hello");
}

// ─────────────────────────────────────────────
// Null terminator invariant
// ─────────────────────────────────────────────

TEST(StringNullTerminator, AlwaysNullTerminated)
{
    String s("abc");
    EXPECT_EQ(s.cstr()[s.size()], '\0');
}

TEST(StringNullTerminator, AfterResize)
{
    String s("hello");
    s.resize(3);
    EXPECT_EQ(s.size(), 3U);
    EXPECT_STREQ(s.cstr(), "hel");
    EXPECT_EQ(s.cstr()[3], '\0');
}

TEST(StringNullTerminator, AfterClear)
{
    String s("hello");
    s.clear();
    EXPECT_TRUE(s.empty());
    EXPECT_EQ(s.cstr()[0], '\0');
}

TEST(StringNullTerminator, AfterPushChar)
{
    String s("hi");
    s.push('!');
    EXPECT_EQ(s.size(), 3U);
    EXPECT_STREQ(s.cstr(), "hi!");
    EXPECT_EQ(s.cstr()[3], '\0');
}

// ─────────────────────────────────────────────
// Equality
// ─────────────────────────────────────────────

TEST(StringEquality, EqualStrings)
{
    String a("hello");
    String b("hello");
    EXPECT_TRUE(a == b);
}

TEST(StringEquality, UnequalStrings)
{
    String a("hello");
    String b("world");
    EXPECT_FALSE(a == b);
}

TEST(StringEquality, UnequalLengths)
{
    String a("hi");
    String b("hi!");
    EXPECT_FALSE(a == b);
}

TEST(StringEquality, AgainstCstr)
{
    String s("hello");
    EXPECT_TRUE(s == "hello");
    EXPECT_FALSE(s == "world");
    EXPECT_FALSE(s == "hell");
}

TEST(StringEquality, AgainstStringView)
{
    String     s("hello");
    StringView view("hello");
    EXPECT_TRUE(s == view);
    EXPECT_FALSE(s == StringView("hell"));
}

// ─────────────────────────────────────────────
// push
// ─────────────────────────────────────────────

TEST(StringPush, PushChar)
{
    String s("ab");
    s.push('c');
    EXPECT_STREQ(s.cstr(), "abc");
    EXPECT_EQ(s.size(), 3U);
}

TEST(StringPush, PushCstr)
{
    String s("hello");
    s.push(" world");
    EXPECT_STREQ(s.cstr(), "hello world");
    EXPECT_EQ(s.size(), 11U);
}

TEST(StringPush, PushString)
{
    String a("hello");
    String b(" world");
    a.push(b);
    EXPECT_STREQ(a.cstr(), "hello world");
}

TEST(StringPush, PushStringView)
{
    String     s("hello");
    StringView view(" world");
    s.push(view);
    EXPECT_STREQ(s.cstr(), "hello world");
}

TEST(StringPush, PushEmpty)
{
    String s("hello");
    s.push("");
    EXPECT_STREQ(s.cstr(), "hello");
    EXPECT_EQ(s.size(), 5U);
}

// ─────────────────────────────────────────────
// operator+
// ─────────────────────────────────────────────

TEST(StringConcat, OperatorPlus)
{
    String a("hello");
    String b(" world");
    String c = a + b;
    EXPECT_STREQ(c.cstr(), "hello world");
    // Originals unchanged
    EXPECT_STREQ(a.cstr(), "hello");
    EXPECT_STREQ(b.cstr(), " world");
}

// ─────────────────────────────────────────────
// replace
// ─────────────────────────────────────────────

TEST(StringReplace, ReplaceAllOccurrences)
{
    String s("hello world");
    s.replace('l', 'r');
    EXPECT_STREQ(s.cstr(), "herro worrd");
}

TEST(StringReplace, ReplaceNonExistentChar)
{
    String s("hello");
    s.replace('z', 'x');
    EXPECT_STREQ(s.cstr(), "hello");
}

TEST(StringReplace, ReplaceInEmptyString)
{
    String s;
    s.replace('a', 'b');
    EXPECT_TRUE(s.empty());
}

TEST(StringFind, LfindFound)
{
    String str("hello");
    EXPECT_EQ(str.lfind('l'), 2U);
}

TEST(StringFind, LfindNotFound)
{
    String str("hello");
    EXPECT_EQ(str.lfind('z'), String::kNpos);
}

TEST(StringFind, RfindFound)
{
    String str("hello");
    EXPECT_EQ(str.rfind('l'), 3U);
}

TEST(StringFind, RfindNotFound)
{
    String str("hello");
    EXPECT_EQ(str.rfind('z'), String::kNpos);
}

// ─────────────────────────────────────────────
// resize / reserve
// ─────────────────────────────────────────────

TEST(StringResize, GrowsSize)
{
    String s("hi");
    s.resize(5);
    EXPECT_EQ(s.size(), 5U);
    EXPECT_EQ(s.cstr()[5], '\0');
}

TEST(StringResize, ShrinksSize)
{
    String s("hello");
    s.resize(2);
    EXPECT_EQ(s.size(), 2U);
    EXPECT_STREQ(s.cstr(), "he");
}

TEST(StringReserve, DoesNotChangeSize)
{
    String s("hi");
    s.reserve(100);
    EXPECT_EQ(s.size(), 2U);
    EXPECT_STREQ(s.cstr(), "hi");
}

// ─────────────────────────────────────────────
// Iterators
// ─────────────────────────────────────────────

TEST(StringIterators, BeginEndSpansContent)
{
    String s("hello");
    EXPECT_EQ(s.end() - s.begin(), 5);
}

TEST(StringIterators, RangeForLoop)
{
    String s("abc");
    int    count = 0;
    for (char c : s)
    {
        (void)c;
        count++;
    }
    EXPECT_EQ(count, 3);
}

TEST(StringIterators, EndExcludesNullByte)
{
    String s("hi");
    EXPECT_EQ(s.cstr()[s.size()], '\0');
    EXPECT_EQ(s.end() - s.begin(), 2);
}

// ─────────────────────────────────────────────
// StackString
// ─────────────────────────────────────────────

TEST(StackString, BasicConstruction)
{
    StackString<16> s("hello");
    EXPECT_EQ(s.size(), 5U);
    EXPECT_STREQ(s.cstr(), "hello");
}

TEST(StackString, PushAndConcat)
{
    StackString<32> s("hello");
    s.push(" world");
    EXPECT_STREQ(s.cstr(), "hello world");
}

TEST(StackString, EqualityWithString)
{
    StackString<16> stack("hello");
    String          heap("hello");
    EXPECT_TRUE(stack == heap);
}

// ─────────────────────────────────────────────
// StringView Construction
// ─────────────────────────────────────────────

TEST(StringViewConstruction, DefaultIsEmpty)
{
    StringView view;
    EXPECT_EQ(view.size(), 0U);
}

TEST(StringViewConstruction, FromCstr)
{
    StringView view("hello");
    EXPECT_EQ(view.size(), 5U);
    EXPECT_EQ(view[0], 'h');
    EXPECT_EQ(view[4], 'o');
}

TEST(StringViewConstruction, FromCstrWithCount)
{
    StringView view("hello world", 5);
    EXPECT_EQ(view.size(), 5U);
    EXPECT_EQ(view[4], 'o');
}

TEST(StringViewConstruction, FromString)
{
    String     s("hello");
    StringView view(s);
    EXPECT_EQ(view.size(), 5U);
    EXPECT_EQ(view[0], 'h');
}

TEST(StringViewConstruction, FromIterators)
{
    String     s("hello");
    StringView view(s.cbegin(), s.cend());
    EXPECT_EQ(view.size(), 5U);
    EXPECT_EQ(view[0], 'h');
}

// ─────────────────────────────────────────────
// StringView lfind / rfind
// ─────────────────────────────────────────────

TEST(StringViewFind, LfindFound)
{
    StringView view("hello");
    EXPECT_EQ(view.lfind('l'), 2U);
}

TEST(StringViewFind, LfindNotFound)
{
    StringView view("hello");
    EXPECT_EQ(view.lfind('z'), StringView::kNpos);
}

TEST(StringViewFind, RfindFound)
{
    StringView view("hello");
    EXPECT_EQ(view.rfind('l'), 3U);
}

TEST(StringViewFind, RfindNotFound)
{
    StringView view("hello");
    EXPECT_EQ(view.rfind('z'), StringView::kNpos);
}

// ─────────────────────────────────────────────
// StringView subview
// ─────────────────────────────────────────────

TEST(StringViewSubview, SubviewWithCount)
{
    StringView view("hello world");
    StringView sub = view.subview(6, 5);
    EXPECT_EQ(sub.size(), 5U);
    EXPECT_TRUE(sub == StringView("world"));
}

TEST(StringViewSubview, SubviewFromIndex)
{
    StringView view("hello world");
    StringView sub = view.subview(6);
    EXPECT_TRUE(sub == StringView("world"));
}

TEST(StringViewSubview, SubviewFromZero)
{
    StringView view("hello");
    StringView sub = view.subview(0, 5);
    EXPECT_TRUE(sub == view);
}

// ─────────────────────────────────────────────
// StringView equality
// ─────────────────────────────────────────────

TEST(StringViewEquality, EqualViews)
{
    StringView a("hello");
    StringView b("hello");
    EXPECT_TRUE(a == b);
}

TEST(StringViewEquality, UnequalViews)
{
    StringView a("hello");
    StringView b("world");
    EXPECT_FALSE(a == b);
}

TEST(StringViewEquality, UnequalLengths)
{
    StringView a("hi");
    StringView b("hi!");
    EXPECT_FALSE(a == b);
}

} // namespace mk
