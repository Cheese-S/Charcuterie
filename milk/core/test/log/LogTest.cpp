#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <core/log/IFormatter.h>
#include <core/log/ISink.h>
#include <core/log/Log.h>
#include <core/filesystem/IPath.h>

namespace mk::log
{

// ─────────────────────────────────────────────
// Helpers
// ─────────────────────────────────────────────

class MockSink: public ISink
{
public:
    MOCK_METHOD(void, write, (LogContext & ctx, StringView msg), (override));
};

namespace
{
// Builds a LogContext with fully controlled fields for formatter tests
LogContext makeCtx(const char* userMsg = "hello",
                   const char* category = "test",
                   const char* filename = "MyFile.cpp",
                   LogLevel    level = LogLevel::eInfo,
                   u16         line = 42,
                   u16         ms = 123,
                   u32         count = 7,
                   u32         tid = 0)
{
    LogContext ctx{};
    ctx.userMsg = StringView(userMsg);
    ctx.category = StringView(category);
    ctx.filename = StringView(filename);
    ctx.level = level;
    ctx.line = line;
    ctx.ms = ms;
    ctx.count = count;
    ctx.tid = tid;

    // Fixed date/time: 2024-03-05 09:07:04
    ctx.tm.tm_year = 124; // 2024 - 1900
    ctx.tm.tm_mon = 2;    // March (0-indexed)
    ctx.tm.tm_mday = 5;
    ctx.tm.tm_hour = 9;
    ctx.tm.tm_min = 7;
    ctx.tm.tm_sec = 4;

    return ctx;
}

std::string bufToStr(const MemoryBuffer& buf)
{
    return std::string(buf.begin(), buf.end());
}

Pad noPad()
{
    return Pad{
        .count = 0,
        .direction = PadDirection::eRight,
    };
}

Pad rightPad(u8 count)
{
    return Pad{ .count = count, .direction = PadDirection::eRight };
}

Pad leftPad(u8 count)
{
    return Pad{ .count = count, .direction = PadDirection::eLeft };
}
} // namespace

// ═════════════════════════════════════════════
// LiteralFormatter
// ═════════════════════════════════════════════

TEST(LiteralFormatter, OutputsLiteralAsIs)
{
    LiteralFormatter fmt(StringView("[literal] "));
    fmt.pad = noPad();
    LogContext   ctx = makeCtx();
    MemoryBuffer buf;
    fmt.format(ctx, buf);
    EXPECT_EQ(bufToStr(buf), "[literal] ");
}

TEST(LiteralFormatter, EmptyLiteral)
{
    LiteralFormatter fmt(StringView(""));
    fmt.pad = noPad();
    LogContext   ctx = makeCtx();
    MemoryBuffer buf;
    fmt.format(ctx, buf);
    EXPECT_EQ(bufToStr(buf), "");
}

// ═════════════════════════════════════════════
// YearFormatter
// ═════════════════════════════════════════════

TEST(YearFormatter, OutputsFullYear)
{
    YearFormatter fmt;
    fmt.pad = noPad();
    LogContext   ctx = makeCtx();
    MemoryBuffer buf;
    fmt.format(ctx, buf);
    EXPECT_EQ(bufToStr(buf), "2024");
}

TEST(YearFormatter, RightPadToWidth)
{
    YearFormatter fmt;
    fmt.pad = rightPad(6);
    LogContext   ctx = makeCtx();
    MemoryBuffer buf;
    fmt.format(ctx, buf);
    EXPECT_EQ(bufToStr(buf), "2024  ");
}

TEST(YearFormatter, LeftPadToWidth)
{
    YearFormatter fmt;
    fmt.pad = leftPad(6);
    LogContext   ctx = makeCtx();
    MemoryBuffer buf;
    fmt.format(ctx, buf);
    EXPECT_EQ(bufToStr(buf), "  2024");
}

// ═════════════════════════════════════════════
// MonthFormatter
// ═════════════════════════════════════════════

TEST(MonthFormatter, SingleDigitMonthPaddedWithZero)
{
    MonthFormatter fmt;
    fmt.pad = noPad();
    LogContext ctx = makeCtx();
    ctx.tm.tm_mon = 0; // January → "01"
    MemoryBuffer buf;
    fmt.format(ctx, buf);
    EXPECT_EQ(bufToStr(buf), "01");
}

TEST(MonthFormatter, TwoDigitMonth)
{
    MonthFormatter fmt;
    fmt.pad = noPad();
    LogContext ctx = makeCtx();
    ctx.tm.tm_mon = 11; // December → "12"
    MemoryBuffer buf;
    fmt.format(ctx, buf);
    EXPECT_EQ(bufToStr(buf), "12");
}

// ═════════════════════════════════════════════
// DayFormatter
// ═════════════════════════════════════════════

TEST(DayFormatter, SingleDigitDayPaddedWithZero)
{
    DayFormatter fmt;
    fmt.pad = noPad();
    LogContext   ctx = makeCtx(); // tm_mday = 5 → "05"
    MemoryBuffer buf;
    fmt.format(ctx, buf);
    EXPECT_EQ(bufToStr(buf), "05");
}

TEST(DayFormatter, TwoDigitDay)
{
    DayFormatter fmt;
    fmt.pad = noPad();
    LogContext ctx = makeCtx();
    ctx.tm.tm_mday = 31;
    MemoryBuffer buf;
    fmt.format(ctx, buf);
    EXPECT_EQ(bufToStr(buf), "31");
}

// ═════════════════════════════════════════════
// HourFormatter
// ═════════════════════════════════════════════

TEST(HourFormatter, SingleDigitHourPadded)
{
    HourFormatter fmt;
    fmt.pad = noPad();
    LogContext   ctx = makeCtx(); // tm_hour = 9 → "09"
    MemoryBuffer buf;
    fmt.format(ctx, buf);
    EXPECT_EQ(bufToStr(buf), "09");
}

TEST(HourFormatter, TwoDigitHour)
{
    HourFormatter fmt;
    fmt.pad = noPad();
    LogContext ctx = makeCtx();
    ctx.tm.tm_hour = 23;
    MemoryBuffer buf;
    fmt.format(ctx, buf);
    EXPECT_EQ(bufToStr(buf), "23");
}

TEST(HourFormatter, MidnightHour)
{
    HourFormatter fmt;
    fmt.pad = noPad();
    LogContext ctx = makeCtx();
    ctx.tm.tm_hour = 0;
    MemoryBuffer buf;
    fmt.format(ctx, buf);
    EXPECT_EQ(bufToStr(buf), "00");
}

// ═════════════════════════════════════════════
// MinuteFormatter
// ═════════════════════════════════════════════

TEST(MinuteFormatter, SingleDigitMinutePadded)
{
    MinuteFormatter fmt;
    fmt.pad = noPad();
    LogContext   ctx = makeCtx(); // tm_min = 7 → "07"
    MemoryBuffer buf;
    fmt.format(ctx, buf);
    EXPECT_EQ(bufToStr(buf), "07");
}

TEST(MinuteFormatter, TwoDigitMinute)
{
    MinuteFormatter fmt;
    fmt.pad = noPad();
    LogContext ctx = makeCtx();
    ctx.tm.tm_min = 59;
    MemoryBuffer buf;
    fmt.format(ctx, buf);
    EXPECT_EQ(bufToStr(buf), "59");
}

// ═════════════════════════════════════════════
// SecondFormatter
// ═════════════════════════════════════════════

TEST(SecondFormatter, SingleDigitSecondPadded)
{
    SecondFormatter fmt;
    fmt.pad = noPad();
    LogContext   ctx = makeCtx(); // tm_sec = 4 → "04"
    MemoryBuffer buf;
    fmt.format(ctx, buf);
    EXPECT_EQ(bufToStr(buf), "04");
}

TEST(SecondFormatter, TwoDigitSecond)
{
    SecondFormatter fmt;
    fmt.pad = noPad();
    LogContext ctx = makeCtx();
    ctx.tm.tm_sec = 59;
    MemoryBuffer buf;
    fmt.format(ctx, buf);
    EXPECT_EQ(bufToStr(buf), "59");
}

// ═════════════════════════════════════════════
// MillisecondFormatter
// ═════════════════════════════════════════════

TEST(MillisecondFormatter, ThreeDigitMs)
{
    MillisecondFormatter fmt;
    fmt.pad = noPad();
    LogContext   ctx = makeCtx(); // ms = 123 → "123"
    MemoryBuffer buf;
    fmt.format(ctx, buf);
    EXPECT_EQ(bufToStr(buf), "123");
}

TEST(MillisecondFormatter, SingleDigitMsPaddedToThree)
{
    MillisecondFormatter fmt;
    fmt.pad = noPad();
    LogContext ctx = makeCtx();
    ctx.ms = 5; // → "005"
    MemoryBuffer buf;
    fmt.format(ctx, buf);
    EXPECT_EQ(bufToStr(buf), "005");
}

TEST(MillisecondFormatter, TwoDigitMsPaddedToThree)
{
    MillisecondFormatter fmt;
    fmt.pad = noPad();
    LogContext ctx = makeCtx();
    ctx.ms = 42; // → "042"
    MemoryBuffer buf;
    fmt.format(ctx, buf);
    EXPECT_EQ(bufToStr(buf), "042");
}

TEST(MillisecondFormatter, ZeroMs)
{
    MillisecondFormatter fmt;
    fmt.pad = noPad();
    LogContext ctx = makeCtx();
    ctx.ms = 0;
    MemoryBuffer buf;
    fmt.format(ctx, buf);
    EXPECT_EQ(bufToStr(buf), "000");
}

// ═════════════════════════════════════════════
// LevelFormatter
// ═════════════════════════════════════════════

TEST(LevelFormatter, InfoLevel)
{
    LevelFormater fmt;
    fmt.pad = noPad();
    LogContext   ctx = makeCtx("", "", "", LogLevel::eInfo);
    MemoryBuffer buf;
    fmt.format(ctx, buf);
    EXPECT_EQ(bufToStr(buf), "I");
}

TEST(LevelFormatter, DebugLevel)
{
    LevelFormater fmt;
    fmt.pad = noPad();
    LogContext   ctx = makeCtx("", "", "", LogLevel::eDebug);
    MemoryBuffer buf;
    fmt.format(ctx, buf);
    EXPECT_EQ(bufToStr(buf), "D");
}

TEST(LevelFormatter, WarningLevel)
{
    LevelFormater fmt;
    fmt.pad = noPad();
    LogContext   ctx = makeCtx("", "", "", LogLevel::eWarning);
    MemoryBuffer buf;
    fmt.format(ctx, buf);
    EXPECT_EQ(bufToStr(buf), "W");
}

TEST(LevelFormatter, ErrorLevel)
{
    LevelFormater fmt;
    fmt.pad = noPad();
    LogContext   ctx = makeCtx("", "", "", LogLevel::eError);
    MemoryBuffer buf;
    fmt.format(ctx, buf);
    EXPECT_EQ(bufToStr(buf), "E");
}

TEST(LevelFormatter, RightPad)
{
    LevelFormater fmt;
    fmt.pad = rightPad(4);
    LogContext   ctx = makeCtx("", "", "", LogLevel::eInfo);
    MemoryBuffer buf;
    fmt.format(ctx, buf);
    EXPECT_EQ(bufToStr(buf), "I   ");
}

TEST(LevelFormatter, LeftPad)
{
    LevelFormater fmt;
    fmt.pad = leftPad(4);
    LogContext   ctx = makeCtx("", "", "", LogLevel::eInfo);
    MemoryBuffer buf;
    fmt.format(ctx, buf);
    EXPECT_EQ(bufToStr(buf), "   I");
}

// ═════════════════════════════════════════════
// CountFormatter
// ═════════════════════════════════════════════

TEST(CountFormatter, PadsToEightDigits)
{
    CountFormatter fmt;
    fmt.pad = noPad();
    LogContext ctx = makeCtx();
    ctx.count = 7;
    MemoryBuffer buf;
    fmt.format(ctx, buf);
    EXPECT_EQ(bufToStr(buf), "00000007");
}

TEST(CountFormatter, LargeCount)
{
    CountFormatter fmt;
    fmt.pad = noPad();
    LogContext ctx = makeCtx();
    ctx.count = 12345678;
    MemoryBuffer buf;
    fmt.format(ctx, buf);
    EXPECT_EQ(bufToStr(buf), "12345678");
}

TEST(CountFormatter, ZeroCount)
{
    CountFormatter fmt;
    fmt.pad = noPad();
    LogContext ctx = makeCtx();
    ctx.count = 0;
    MemoryBuffer buf;
    fmt.format(ctx, buf);
    EXPECT_EQ(bufToStr(buf), "00000000");
}

// ═════════════════════════════════════════════
// UserMsgFormatter
// ═════════════════════════════════════════════

TEST(UserMsgFormatter, OutputsUserMessage)
{
    UserMsgFormatter fmt;
    fmt.pad = noPad();
    LogContext   ctx = makeCtx("hello world");
    MemoryBuffer buf;
    fmt.format(ctx, buf);
    EXPECT_EQ(bufToStr(buf), "hello world");
}

TEST(UserMsgFormatter, EmptyMessage)
{
    UserMsgFormatter fmt;
    fmt.pad = noPad();
    LogContext   ctx = makeCtx("");
    MemoryBuffer buf;
    fmt.format(ctx, buf);
    EXPECT_EQ(bufToStr(buf), "");
}

TEST(UserMsgFormatter, RightPad)
{
    UserMsgFormatter fmt;
    fmt.pad = rightPad(8);
    LogContext   ctx = makeCtx("hi");
    MemoryBuffer buf;
    fmt.format(ctx, buf);
    EXPECT_EQ(bufToStr(buf), "hi      ");
}

TEST(UserMsgFormatter, LeftPad)
{
    UserMsgFormatter fmt;
    fmt.pad = leftPad(8);
    LogContext   ctx = makeCtx("hi");
    MemoryBuffer buf;
    fmt.format(ctx, buf);
    EXPECT_EQ(bufToStr(buf), "      hi");
}

// ═════════════════════════════════════════════
// CategoryFormatter
// ═════════════════════════════════════════════

TEST(CategoryFormatter, OutputsCategory)
{
    CategoryFormatter fmt;
    fmt.pad = noPad();
    LogContext   ctx = makeCtx("", "render");
    MemoryBuffer buf;
    fmt.format(ctx, buf);
    EXPECT_EQ(bufToStr(buf), "render");
}

TEST(CategoryFormatter, RightPad)
{
    CategoryFormatter fmt;
    fmt.pad = rightPad(10);
    LogContext   ctx = makeCtx("", "net");
    MemoryBuffer buf;
    fmt.format(ctx, buf);
    EXPECT_EQ(bufToStr(buf), "net       ");
}

TEST(CategoryFormatter, LeftPad)
{
    CategoryFormatter fmt;
    fmt.pad = leftPad(10);
    LogContext   ctx = makeCtx("", "net");
    MemoryBuffer buf;
    fmt.format(ctx, buf);
    EXPECT_EQ(bufToStr(buf), "       net");
}

// ═════════════════════════════════════════════
// LineFormatter
// ═════════════════════════════════════════════

TEST(LineFormatter, OutputsLineNumber)
{
    LineFormatter fmt;
    fmt.pad = noPad();
    LogContext ctx = makeCtx();
    ctx.line = 42;
    MemoryBuffer buf;
    fmt.format(ctx, buf);
    EXPECT_EQ(bufToStr(buf), "42");
}

TEST(LineFormatter, LargeLineNumber)
{
    LineFormatter fmt;
    fmt.pad = noPad();
    LogContext ctx = makeCtx();
    ctx.line = 9999;
    MemoryBuffer buf;
    fmt.format(ctx, buf);
    EXPECT_EQ(bufToStr(buf), "9999");
}

TEST(LineFormatter, RightPad)
{
    LineFormatter fmt;
    fmt.pad = rightPad(6);
    LogContext ctx = makeCtx();
    ctx.line = 42;
    MemoryBuffer buf;
    fmt.format(ctx, buf);
    EXPECT_EQ(bufToStr(buf), "42    ");
}

// ═════════════════════════════════════════════
// FilenameFormatter
// ═════════════════════════════════════════════

TEST(FilenameFormatter, OutputsFilename)
{
    FilenameFormatter fmt;
    fmt.pad = noPad();
    LogContext   ctx = makeCtx("", "", "MyFile.cpp");
    MemoryBuffer buf;
    fmt.format(ctx, buf);
    EXPECT_EQ(bufToStr(buf), "MyFile.cpp");
}

TEST(FilenameFormatter, LeftPad)
{
    FilenameFormatter fmt;
    fmt.pad = leftPad(14);
    LogContext   ctx = makeCtx("", "", "main.cpp");
    MemoryBuffer buf;
    fmt.format(ctx, buf);
    EXPECT_EQ(bufToStr(buf), "      main.cpp");
}

// ═════════════════════════════════════════════
// ThreadIdFormatter
// ═════════════════════════════════════════════

TEST(ThreadIdFormatter, OutputsThreadId)
{
    ThreadIdFormatter fmt;
    fmt.pad = noPad();
    LogContext ctx = makeCtx();
    ctx.tid = 210;
    MemoryBuffer buf;
    fmt.format(ctx, buf);
    EXPECT_EQ(bufToStr(buf), "210");
}

TEST(ThreadIdFormatter, ZeroThreadId)
{
    ThreadIdFormatter fmt;
    fmt.pad = noPad();
    LogContext ctx = makeCtx();
    ctx.tid = 0;
    MemoryBuffer buf;
    fmt.format(ctx, buf);
    EXPECT_EQ(bufToStr(buf), "0");
}

// ═════════════════════════════════════════════
// compilePattern
// ═════════════════════════════════════════════

TEST(CompilePattern, ValidPatternCompilesOk)
{
    FixedVector<IFormatterPtr, 256> flagMap;
    flagMap.resize(256);
    flagMap['v'] = makeUnique<UserMsgFormatter>();
    flagMap['l'] = makeUnique<LevelFormater>();

    LogSystem::FormatterVector formatters;
    Result result = compilePattern(StringView("%l %v"), flagMap, formatters);
    EXPECT_EQ(result, Result::eOk);
    // LevelFormatter + LiteralFormatter(" ") + UserMsgFormatter
    EXPECT_EQ(formatters.size(), 3U);
}

TEST(CompilePattern, UnknownFlagReturnsError)
{
    FixedVector<IFormatterPtr, 256> flagMap;
    flagMap.resize(256);

    LogSystem::FormatterVector formatters;
    Result result = compilePattern(StringView("%Z"), flagMap, formatters);
    EXPECT_EQ(result, Result::eInvalidParam);
}

TEST(CompilePattern, PurelyLiteralPattern)
{
    FixedVector<IFormatterPtr, 256> flagMap;
    flagMap.resize(256);

    LogSystem::FormatterVector formatters;
    Result result = compilePattern(StringView("[prefix] "), flagMap, formatters);
    EXPECT_EQ(result, Result::eOk);
    EXPECT_EQ(formatters.size(), 1U);
}

TEST(CompilePattern, LeftPaddingSpecifierParsed)
{
    FixedVector<IFormatterPtr, 256> flagMap;
    flagMap.resize(256);
    flagMap['v'] = makeUnique<UserMsgFormatter>();

    LogSystem::FormatterVector formatters;
    Result result = compilePattern(StringView("%-10v"), flagMap, formatters);
    EXPECT_EQ(result, Result::eOk);
    ASSERT_EQ(formatters.size(), 1U);
    EXPECT_EQ(formatters[0]->pad.count, 10);
    EXPECT_EQ(formatters[0]->pad.direction, PadDirection::eLeft);
}

TEST(CompilePattern, RightPaddingSpecifierParsed)
{
    FixedVector<IFormatterPtr, 256> flagMap;
    flagMap.resize(256);
    flagMap['v'] = makeUnique<UserMsgFormatter>();

    LogSystem::FormatterVector formatters;
    Result result = compilePattern(StringView("%8v"), flagMap, formatters);
    EXPECT_EQ(result, Result::eOk);
    ASSERT_EQ(formatters.size(), 1U);
    EXPECT_EQ(formatters[0]->pad.count, 8);
    EXPECT_EQ(formatters[0]->pad.direction, PadDirection::eRight);
}

TEST(CompilePattern, MultipleFlagsAndLiterals)
{
    FixedVector<IFormatterPtr, 256> flagMap;
    flagMap.resize(256);
    flagMap['Y'] = makeUnique<YearFormatter>();
    flagMap['l'] = makeUnique<LevelFormater>();
    flagMap['v'] = makeUnique<UserMsgFormatter>();

    LogSystem::FormatterVector formatters;
    // "[" + Year + "] " + Level + " - " + UserMsg = 6 formatters
    Result result = compilePattern(StringView("[%Y] %l - %v"), flagMap, formatters);
    EXPECT_EQ(result, Result::eOk);
    EXPECT_EQ(formatters.size(), 6U);
}

// ═════════════════════════════════════════════
// LogManager — makeLogManager
// ═════════════════════════════════════════════

TEST(LogManagerCreation, NoSinksReturnsError)
{
    UniquePtr<LogSystem>          manager;
    VectorView<ISinkPtr>          emptySinks;
    VectorView<FlagFormatterPair> noUserFormatters;
    Result                        result = LogSystem::makeLogSystem(StringView("%l %v"),
                                             emptySinks,
                                             noUserFormatters,
                                             manager);
    EXPECT_EQ(result, Result::eInvalidParam);
    EXPECT_FALSE(manager);
}

TEST(LogManagerCreation, ValidPatternAndSinkCreatesManager)
{
    Vector<ISinkPtr> sinks;
    sinks.push(makeUnique<ConsoleSink>());
    VectorView<FlagFormatterPair> noUserFormatters;
    UniquePtr<LogSystem>          manager;
    Result                        result =
        LogSystem::makeLogSystem(StringView("[%l] %v"), sinks, noUserFormatters, manager);
    EXPECT_EQ(result, Result::eOk);
    EXPECT_TRUE(manager);
}

TEST(LogManagerCreation, InvalidPatternFlagReturnsError)
{
    Vector<ISinkPtr> sinks;
    sinks.push(makeUnique<ConsoleSink>());
    VectorView<FlagFormatterPair> noUserFormatters;
    UniquePtr<LogSystem>          manager;
    Result                        result =
        LogSystem::makeLogSystem(StringView("%Z"), sinks, noUserFormatters, manager);
    EXPECT_EQ(result, Result::eInvalidParam);
    EXPECT_FALSE(manager);
}

// ═════════════════════════════════════════════
// FileSink
// ═════════════════════════════════════════════

class FileSinkTest: public ::testing::Test
{
protected:
    void SetUp() override
    {
        testPath_ = fs::Path("mk_log_sink_test.txt");
    }
    void TearDown() override
    {
        MK_IGNORE_RET_VAL
        fs::removeFile(testPath_);
    }

    fs::Path testPath_ = fs::Path("");
};

TEST_F(FileSinkTest, MakeFileSinkCreatesFile)
{
    ISinkPtr sink;
    Result   result = FileSink::makeFileSink(testPath_, sink);
    EXPECT_EQ(result, Result::eOk);
    EXPECT_TRUE(sink);
    EXPECT_TRUE(fs::fileExist(testPath_));
}

TEST_F(FileSinkTest, WritePersistsToFile)
{
    ISinkPtr sink;
    ASSERT_EQ(FileSink::makeFileSink(testPath_, sink), Result::eOk);

    LogContext ctx = makeCtx("written to file");
    sink->write(ctx, StringView("written to file\n"));
    sink.reset(); // flush / close

    fs::IFileHandle* handle = nullptr;
    ASSERT_EQ(fs::openFile(testPath_, fs::FileAccessMode::eRead, &handle), Result::eOk);
    String content;
    content.resize(sizeof("written to file\n") - 1);
    MK_IGNORE_RET_VAL
    handle->read(content);
    delete handle;

    EXPECT_TRUE(content == "written to file\n");
}

} // namespace mk::log
