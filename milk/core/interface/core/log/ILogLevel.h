#pragma once
#include <core/IType.h>

namespace mk::log
{
enum class LogLevel : u8
{
    eInfo = 0,
    eDebug = 1,
    eWarning = 2,
    eError = 3,

    eCount = 4,
};
} // namespace mk::log
