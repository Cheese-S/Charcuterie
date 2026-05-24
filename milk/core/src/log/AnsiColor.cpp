#include <core/log/AnsiColor.h>
#include <core/log/ILogLevel.h>
#include <core/IAssert.h>

namespace mk::log
{

AnsiColor toAnsiColor(LogLevel level)
{
    switch (level)
    {
    case LogLevel::eInfo:
        return AnsiColor::eGreen;
    case LogLevel::eDebug:
        return AnsiColor::eBlue;
    case LogLevel::eWarning:
        return AnsiColor::eYellow;
    case LogLevel::eError:
        return AnsiColor::eRed;
    case LogLevel::eCount:
        MK_ASSERT_UNREACHABLE();
        return AnsiColor::eGreen;
    }
}

const char* toEscapeCode(AnsiColor color)
{
    switch (color)
    {
    case AnsiColor::eGreen:
        return "\033[32m";
    case AnsiColor::eBlue:
        return "\033[34m";
    case AnsiColor::eYellow:
        return "\033[33m";
    case AnsiColor::eRed:
        return "\033[31m";
    case AnsiColor::eReset:
        return "\033[0m";
    }
};
} // namespace mk::log
