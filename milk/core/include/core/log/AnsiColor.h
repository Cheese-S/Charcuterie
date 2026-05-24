#pragma once
#include <core/IType.h>

namespace mk::log
{
enum class LogLevel : u8;

enum class AnsiColor
{
    eGreen,
    eBlue,
    eYellow,
    eRed,

    eReset
};

AnsiColor   toAnsiColor(LogLevel level);
const char* toEscapeCode(AnsiColor color);

} // namespace mk::log
