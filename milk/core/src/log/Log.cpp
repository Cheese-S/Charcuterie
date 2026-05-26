#include <cctype>
#include <chrono>
#include <core/IMacro.h>
#include <core/IResult.h>
#include <core/log/AnsiColor.h>
#include <core/log/IFormatter.h>
#include <core/log/ILog.h>
#include <core/log/ISink.h>
#include <core/log/Log.h>

namespace mk::log::details
{
// TODO(Cheese_S): implement color for this as well
void rawLogImpl(LogLevel level, std::source_location sourceLocation, const char* view)
{
    FILE* stream = stdout;
    char  levelChar = 'I';
    switch (level)
    {
    case LogLevel::eInfo:
        levelChar = 'I';
        break;
    case LogLevel::eDebug:
        levelChar = 'D';
        break;
    case LogLevel::eWarning:
        levelChar = 'W';
        break;
    case LogLevel::eError:
        levelChar = 'E';
        stream = stderr;
        break;
    default:
        MK_ASSERT(false);
        break;
    }

    const char* color = toEscapeCode(toAnsiColor(level));
    const char* reset = toEscapeCode(AnsiColor::eReset);

    std::fprintf(stream,
                 "%s[raw] %c: %s [%s:%u]%s\n",
                 color,
                 levelChar,
                 view,
                 sourceLocation.file_name(),
                 static_cast<u32>(sourceLocation.line()),
                 reset);
    std::fflush(stream);
}
}; // namespace mk::log::details

namespace mk::log
{

namespace
{

// TODO(Cheese_S):
void registerFlagFormatters(VectorView<IFormatterPtr>     map,
                            VectorView<FlagFormatterPair> userFlagFormatters)
{
    for (auto& flagFormatter : userFlagFormatters)
    {
        map[flagFormatter.flag] = std::move(flagFormatter.formatter);
    }

    if (!map['Y'])
    {
        map['Y'] = makeUnique<YearFormatter>();
    }
    if (!map['M'])
    {
        map['M'] = makeUnique<MonthFormatter>();
    }
    if (!map['D'])
    {
        map['D'] = makeUnique<DayFormatter>();
    }
    if (!map['h'])
    {
        map['h'] = makeUnique<HourFormatter>();
    }
    if (!map['m'])
    {
        map['m'] = makeUnique<MinuteFormatter>();
    }
    if (!map['s'])
    {
        map['s'] = makeUnique<SecondFormatter>();
    }
    if (!map['l'])
    {
        map['l'] = makeUnique<LevelFormater>();
    }
    if (!map['N'])
    {
        map['N'] = makeUnique<CountFormatter>();
    }
    if (!map['v'])
    {
        map['v'] = makeUnique<UserMsgFormatter>();
    }
    if (!map['c'])
    {
        map['c'] = makeUnique<CategoryFormatter>();
    }
    if (!map['e'])
    {
        map['e'] = makeUnique<MillisecondFormatter>();
    }
    if (!map['#'])
    {
        map['#'] = makeUnique<LineFormatter>();
    }
    if (!map['F'])
    {
        map['F'] = makeUnique<FilenameFormatter>();
    }
    if (!map['t'])
    {
        map['t'] = makeUnique<ThreadIdFormatter>();
    }
}

StringView getFilename(StringView fullFilename)
{
    // TODO(Cheese_S): make cross platform?
    usize idx = fullFilename.rfind('\\') + 1;
    if (idx == StringView::kNpos)
    {
        idx = 0;
    }
    return fullFilename.subview(idx);
}

}; // namespace
Result LogSystem::makeLogSystem(LogSystemConfig config, UniquePtr<LogSystem>& outSystem)
{
    if (config.sinks.empty())
    {
        return Result::eInvalidParam;
    }

    FixedVector<IFormatterPtr, 256> flagFomartterMap;
    flagFomartterMap.resize(256);
    LogSystem::FormatterVector formatters;
    registerFlagFormatters(flagFomartterMap, config.userFlagFormatters);
    MK_RETURN_IF_NOT_OK(compilePattern(config.pattern, flagFomartterMap, formatters));

    outSystem = makeUnique<LogSystem>(LogSystemPasskey{},
                                      formatters,
                                      config.sinks,
                                      config.pattern);
    return Result::eOk;
}

LogSystem::LogSystem([[maybe_unused]] LogSystemPasskey key,
                     VectorView<IFormatterPtr>         formatters,
                     VectorView<ISinkPtr>              sinks,
                     StringView pattern): pattern_(pattern), logCount_(0)
{
    for (auto& formatter : formatters)
    {
        formatters_.push(std::move(formatter));
    }

    for (auto& sink : sinks)
    {
        MK_ASSERT(!!sink);
        sinks_.push(std::move(sink));
    }
}

void LogSystem::logImpl(LogCategory          category,
                        LogLevel             level,
                        std::source_location sourceLocation,
                        StringView           msg)
{
    std::chrono::time_point now = std::chrono::system_clock::now();
    std::time_t             nowTime = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::time_point_cast<std::chrono::milliseconds>(now);

    LogContext ctx = {
        .count = logCount_.fetch_add(1, std::memory_order::memory_order_relaxed),
        .userMsg = msg,
        .category = category,
        .filename = getFilename(sourceLocation.file_name()),
        .level = level,
        .tid = 0,
        .ms = static_cast<u16>(ms.time_since_epoch().count() % 1000),
        .line = static_cast<u16>(sourceLocation.line()),
        .tm = {},
    };

    localtime_s(&ctx.tm, &nowTime);

    MemoryBuffer userMsg;

    for (const auto& formatter : formatters_)
    {
        formatter->format(ctx, userMsg);
    }

    userMsg.push_back('\n');
    userMsg.push_back('\0');

    for (const auto& sink : sinks_)
    {
        sink->write(ctx, StringView(userMsg.begin(), userMsg.size()));
    }
}

Result compilePattern(StringView                  pattern,
                      VectorView<IFormatterPtr>   flagFormatterMap,
                      LogSystem::FormatterVector& formatters)
{
    Lexer lex = {
        .i = 0,
        .pattern = pattern,
    };

    while (lex.i < pattern.size())
    {
        switch (lex.pattern[lex.i])
        {
        case '%':
            lex.i++; // consume '%'
            Pad pad;
            MK_RETURN_IF_NOT_OK(lexPadding(lex, pad));
            MK_RETURN_IF_NOT_OK(lexFlagFormatter(lex, flagFormatterMap, formatters, pad));
            break;
        default:
            lexLiteralFormatter(lex, formatters);
            break;
        }
    }

    return Result::eOk;
};

Result lexPadding(Lexer& lex, Pad& pad)
{
    pad.direction = PadDirection::eRight;
    if (lex.pattern[lex.i] == '-')
    {
        pad.direction = PadDirection::eLeft;
        lex.i++;
    }

    u16 count;
    while (isdigit(lex.pattern[lex.i]))
    {
        count = count * 10 + lex.pattern[lex.i] - '0';
        if (count > std::numeric_limits<u8>::max())
        {
            return Result::eOutOfRange;
        }
        lex.i++;
    }
    pad.count = static_cast<u8>(count);

    return Result::eOk;
}

Result lexFlagFormatter(Lexer&                      lex,
                        VectorView<IFormatterPtr>   flagFomartterMap,
                        LogSystem::FormatterVector& formatters,
                        Pad                         pad)
{
    if (lex.i >= lex.pattern.size())
    {
        return Result::eInvalidParam;
    }

    char flag = lex.pattern[lex.i++];
    if (!flagFomartterMap[flag])
    {
        MK_RAW_LOG_ERROR("Failed to find flag formatter for flag: {}", flag);
        fprintf(stderr, "Failed to find flag formatter for flag: %c\n", flag);
        return Result::eInvalidParam;
    }
    const auto& formatter = formatters.push(std::move(flagFomartterMap[flag]));
    formatter->pad = pad;

    return Result::eOk;
};

void lexLiteralFormatter(Lexer& lex, LogSystem::FormatterVector& formatters)
{
    usize begin = lex.i;

    while (lex.i < lex.pattern.size() && lex.pattern[lex.i] != '%')
    {
        lex.i++;
    }

    MK_ASSERT(begin != lex.i);
    formatters.push(
        makeUnique<LiteralFormatter>(lex.pattern.subview(begin, lex.i - begin)));
};

} // namespace mk::log
