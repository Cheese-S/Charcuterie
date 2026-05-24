#include <gtest/gtest.h>
#include <core/filesystem/IPath.h>

namespace mk::fs
{

// ─────────────────────────────────────────────
// Construction
// ─────────────────────────────────────────────

TEST(PathConstruction, FromCstr)
{
    Path p("foo/bar");
    EXPECT_STREQ(p.cstr(), "foo/bar");
}

TEST(PathConstruction, FromString)
{
    String s("foo/bar");
    Path   p(s);
    EXPECT_STREQ(p.cstr(), "foo/bar");
}

TEST(PathConstruction, FromStringMove)
{
    String s("foo/bar");
    Path   p(std::move(s));
    EXPECT_STREQ(p.cstr(), "foo/bar");
}

TEST(PathConstruction, NormalizesWindowsSeparators)
{
    Path p("foo\\bar\\baz");
    EXPECT_STREQ(p.cstr(), "foo/bar/baz");
}

TEST(PathConstruction, MixedSeparatorsNormalized)
{
    Path p("foo\\bar/baz\\qux");
    EXPECT_STREQ(p.cstr(), "foo/bar/baz/qux");
}

TEST(PathConstruction, CopyConstructor)
{
    Path a("foo/bar");
    Path b = a;
    EXPECT_STREQ(b.cstr(), "foo/bar");
}

TEST(PathConstruction, MoveConstructor)
{
    Path a("foo/bar");
    Path b = std::move(a);
    EXPECT_STREQ(b.cstr(), "foo/bar");
}

// ─────────────────────────────────────────────
// isAbsolute
// ─────────────────────────────────────────────

TEST(PathIsAbsolute, AbsoluteForwardSlash)
{
    Path p("/foo/bar");
    EXPECT_TRUE(p.isAbsolute());
}

TEST(PathIsAbsolute, AbsoluteWindowsStyleNormalized)
{
    // After normalization '\' becomes '/', so leading '\' becomes absolute
    Path p("\\foo\\bar");
    EXPECT_TRUE(p.isAbsolute());
}

TEST(PathIsAbsolute, RelativeIsNotAbsolute)
{
    Path p("foo/bar");
    EXPECT_FALSE(p.isAbsolute());
}

TEST(PathIsAbsolute, EmptyIsNotAbsolute)
{
    Path p("");
    EXPECT_FALSE(p.isAbsolute());
}

// ─────────────────────────────────────────────
// operator/ with Path
// ─────────────────────────────────────────────

TEST(PathJoin, RelativeJoinRelative)
{
    Path p("foo");
    Path q("bar");
    p / q;
    EXPECT_STREQ(p.cstr(), "foo/bar");
}

TEST(PathJoin, RelativeJoinAbsoluteReplacesPath)
{
    Path p("foo/bar");
    Path q("/baz");
    p / q;
    EXPECT_STREQ(p.cstr(), "/baz");
}

TEST(PathJoin, JoinEmptyPathIsNoop)
{
    Path p("foo/bar");
    Path q("");
    p / q;
    EXPECT_STREQ(p.cstr(), "foo/bar");
}

TEST(PathJoin, ChainedJoins)
{
    Path p("a");
    Path q("b");
    Path r("c");
    p / q / r;
    EXPECT_STREQ(p.cstr(), "a/b/c");
}

TEST(PathJoin, AbsoluteJoinRelative)
{
    Path p("/foo");
    Path q("bar");
    p / q;
    EXPECT_STREQ(p.cstr(), "/foo/bar");
}

// ─────────────────────────────────────────────
// operator/ with const char*
// ─────────────────────────────────────────────

TEST(PathJoinCstr, RelativeJoinRelativeCstr)
{
    Path p("foo");
    p / "bar";
    EXPECT_STREQ(p.cstr(), "foo/bar");
}

TEST(PathJoinCstr, RelativeJoinAbsoluteCstrReplacesPath)
{
    Path p("foo/bar");
    p / "/baz";
    EXPECT_STREQ(p.cstr(), "/baz");
}

TEST(PathJoinCstr, RelativeJoinWindowsAbsoluteCstrReplacesPath)
{
    Path p("foo/bar");
    p / "\\baz";
    EXPECT_STREQ(p.cstr(), "/baz");
}

TEST(PathJoinCstr, JoinEmptyCstrIsNoop)
{
    Path p("foo/bar");
    p / "";
    EXPECT_STREQ(p.cstr(), "foo/bar");
}

TEST(PathJoinCstr, WindowsSeparatorNormalizedOnJoin)
{
    Path p("foo");
    p / "bar\\baz";
    EXPECT_STREQ(p.cstr(), "foo/bar/baz");
}

TEST(PathJoinCstr, ChainedCstrJoins)
{
    Path p("a");
    p / "b" / "c";
    EXPECT_STREQ(p.cstr(), "a/b/c");
}

TEST(PathSuffix, HasSuffix)
{
    Path p("abc.aaa.txt");
    EXPECT_STREQ(p.suffix().begin(), "txt");
}

TEST(PathSuffix, DoesNotHaveSuffix)
{
    Path p("abcaaa");
    EXPECT_STREQ(p.suffix().begin(), "");
}

} // namespace mk::fs
