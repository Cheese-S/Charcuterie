#include <core/log/IFormatter.h>
#include <core/log/Log.h>
#include <fmt/format.h>
#include <core/IAssert.h>

namespace
{
template<typename T>
    requires std::is_integral_v<T>
inline void pushInterger(mk::log::MemoryBuffer& dst, T val)
{
    fmt::format_int i(val);
    dst.append(i);
}

template<typename T>
    requires std::is_unsigned_v<T>
void padTo2(mk::log::MemoryBuffer& dst, T val)
{
    if (val < 100)
    {
        dst.push_back('0' + val / 10);
        dst.push_back('0' + val % 10);
        return;
    }
    MK_ASSERT(false);
}

template<typename T>
    requires std::is_unsigned_v<T>
void padTo3(mk::log::MemoryBuffer& dst, T val)
{
    if (val < 1000)
    {
        dst.push_back('0' + val / 100);
        val %= 100;
        dst.push_back('0' + val / 10);
        dst.push_back('0' + val % 10);
        return;
    }
    MK_ASSERT(false);
}

template<typename T>
    requires std::is_integral_v<T>
void padToNPushInteger(mk::log::MemoryBuffer& dst, T val, size_t n)
{
    fmt::format_int i(val);
    for (size_t count = i.size(); count < n; count++)
    {
        dst.push_back('0');
    }
    dst.append(i);
}

}; // namespace

namespace mk::log
{

namespace
{
class ScopedPadder
{
public:
    ScopedPadder(MemoryBuffer& dst, Pad pad, u16 len);
    ~ScopedPadder();

private:
    MemoryBuffer& dst_;
    Pad           pad_;
    u16           len_;
};

ScopedPadder::ScopedPadder(MemoryBuffer& dst, Pad pad, u16 len):
    dst_(dst), pad_(pad), len_(len)
{
    if (pad_.direction == PadDirection::eLeft)
    {
        i32 shouldPadCount = pad_.count - len_;
        while (shouldPadCount > 0)
        {
            dst_.push_back(' ');
            shouldPadCount--;
        }
    }
}

ScopedPadder::~ScopedPadder()
{
    if (pad_.direction == PadDirection::eRight)
    {
        i32 shouldPadCount = pad_.count - len_;
        while (shouldPadCount > 0)
        {
            dst_.push_back(' ');
            shouldPadCount--;
        }
    }
}
} // namespace

LiteralFormatter::LiteralFormatter(StringView literal): literal_(literal) {}

// --------------------------------------------------------------------------
void LiteralFormatter::format([[maybe_unused]] LogContext& ctx, MemoryBuffer& dst)
{
    dst.append(literal_);
}

// --------------------------------------------------------------------------
void YearFormatter::format(LogContext& ctx, MemoryBuffer& dst)
{
    ScopedPadder padder(dst, pad, 4);
    pushInterger<int>(dst, ctx.tm.tm_year + 1900);
}

// --------------------------------------------------------------------------
void MonthFormatter::format(LogContext& ctx, MemoryBuffer& dst)
{
    ScopedPadder padder(dst, pad, 2);
    padTo2<u8>(dst, ctx.tm.tm_mon + 1);
}

// --------------------------------------------------------------------------
void DayFormatter::format(LogContext& ctx, MemoryBuffer& dst)
{
    ScopedPadder padder(dst, pad, 2);
    padTo2<u8>(dst, ctx.tm.tm_mday);
}

// --------------------------------------------------------------------------
void HourFormatter::format(LogContext& ctx, MemoryBuffer& dst)
{
    ScopedPadder padder(dst, pad, 2);
    padTo2<u8>(dst, ctx.tm.tm_hour);
}

// --------------------------------------------------------------------------
void MinuteFormatter::format(LogContext& ctx, MemoryBuffer& dst)
{
    ScopedPadder padder(dst, pad, 2);
    padTo2<u8>(dst, ctx.tm.tm_min);
}

// --------------------------------------------------------------------------
void SecondFormatter::format(LogContext& ctx, MemoryBuffer& dst)
{
    ScopedPadder padder(dst, pad, 2);
    padTo2<u8>(dst, ctx.tm.tm_sec);
}

// --------------------------------------------------------------------------
void MillisecondFormatter::format(LogContext& ctx, MemoryBuffer& dst)
{
    ScopedPadder padder(dst, pad, 3);
    padTo3(dst, ctx.ms);
}

// --------------------------------------------------------------------------
void LevelFormater::format(LogContext& ctx, MemoryBuffer& dst)
{
    ScopedPadder padder(dst, pad, 1);
    switch (ctx.level)
    {
    case LogLevel::eInfo:
        dst.push_back('I');
        break;
    case LogLevel::eDebug:
        dst.push_back('D');
        break;
    case LogLevel::eWarning:
        dst.push_back('W');
        break;
    case LogLevel::eError:
        dst.push_back('E');
        break;
    default:
        MK_ASSERT(false);
        break;
    }
}

// --------------------------------------------------------------------------
void CountFormatter::format(LogContext& ctx, MemoryBuffer& dst)
{
    ScopedPadder padder(dst, pad, 8);
    padToNPushInteger(dst, ctx.count, 8);
}

// --------------------------------------------------------------------------
void UserMsgFormatter::format(LogContext& ctx, MemoryBuffer& dst)
{
    ScopedPadder padder(dst, pad, ctx.userMsg.size());
    dst.append(ctx.userMsg);
}

// --------------------------------------------------------------------------
void CategoryFormatter::format(LogContext& ctx, MemoryBuffer& dst)
{
    ScopedPadder padder(dst, pad, ctx.category.size());
    dst.append(ctx.category);
}

// --------------------------------------------------------------------------
void LineFormatter::format(LogContext& ctx, MemoryBuffer& dst)
{
    // Hopefully we don't have 10k lines...
    ScopedPadder padder(dst, pad, fmt::detail::count_digits<4>(ctx.line));
    pushInterger(dst, ctx.line);
}

// --------------------------------------------------------------------------
void FilenameFormatter::format(LogContext& ctx, MemoryBuffer& dst)
{
    ScopedPadder padder(dst, pad, ctx.filename.size());
    dst.append(ctx.filename);
}

// --------------------------------------------------------------------------
void ThreadIdFormatter::format(LogContext& ctx, MemoryBuffer& dst)
{
    ScopedPadder padder(dst, pad, fmt::detail::count_digits<3>(ctx.tid));
    pushInterger(dst, ctx.tid);
}

} // namespace mk::log
