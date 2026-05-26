#pragma once
#include <core/container/IString.h>
#include <core/log/ILog.h>
#include <fmt/format.h>

namespace mk::log
{
// forward declare
struct LogContext;

enum class PadDirection : u8
{
    eLeft,
    eRight
};

struct Pad
{
    u8           count;
    PadDirection direction;
};

class IFormatter
{
public:
    virtual ~IFormatter() = default;
    virtual void format(LogContext& ctx, MemoryBuffer& dst) = 0;

    Pad pad;
};

class LiteralFormatter: public IFormatter
{
public:
    LiteralFormatter(StringView literalView);

    void format(LogContext& ctx, MemoryBuffer& dst) override;

private:
    StringView literal_;
};

// %Y
class YearFormatter: public IFormatter
{
public:
    void format(LogContext& ctx, MemoryBuffer& dst) override;
};

// %M
class MonthFormatter: public IFormatter
{
public:
    void format(LogContext& ctx, MemoryBuffer& dst) override;
};

// %D
class DayFormatter: public IFormatter
{
public:
    void format(LogContext& ctx, MemoryBuffer& dst) override;
};

// %h
class HourFormatter: public IFormatter
{
public:
    void format(LogContext& ctx, MemoryBuffer& dst) override;
};

// %m
class MinuteFormatter: public IFormatter
{
public:
    void format(LogContext& ctx, MemoryBuffer& dst) override;
};

// %s
class SecondFormatter: public IFormatter
{
public:
    void format(LogContext& ctx, MemoryBuffer& dst) override;
};

// %e
class MillisecondFormatter: public IFormatter
{
public:
    void format(LogContext& ctx, MemoryBuffer& dst) override;
};

// %l
class LevelFormater: public IFormatter
{
public:
    void format(LogContext& ctx, MemoryBuffer& dst) override;
};

// %N
class CountFormatter: public IFormatter
{
public:
    void format(LogContext& ctx, MemoryBuffer& dst) override;
};

// %v
class UserMsgFormatter: public IFormatter
{
public:
    void format(LogContext& ctx, MemoryBuffer& dst) override;
};

// %c
class CategoryFormatter: public IFormatter
{
public:
    void format(LogContext& ctx, MemoryBuffer& dst) override;
};

// %#
class LineFormatter: public IFormatter
{
public:
    void format(LogContext& ctx, MemoryBuffer& dst) override;
};

// %F
class FilenameFormatter: public IFormatter
{
public:
    void format(LogContext& ctx, MemoryBuffer& dst) override;
};

// %t
class ThreadIdFormatter: public IFormatter
{
public:
    void format(LogContext& ctx, MemoryBuffer& dst) override;
};

} // namespace mk::log
