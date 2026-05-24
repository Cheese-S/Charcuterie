#pragma once
#include <core/IType.h>

namespace mk::cc
{
enum class JobPriority : u8
{
    eHigh = 0,
    eLow = 1,

    eBackgroundHigh = 2,
    eBackgroundLow = 3,

    eCount = 4,
};

enum JobPriorityGroup : u8
{
    eForeground = 0,
    eBackground,

    eCount
};

}; // namespace mk::cc
