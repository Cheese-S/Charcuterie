#include <core/util/IClock.h>

namespace mk::util
{

TimePoint toTimePoint(Duration duration)
{
    if (duration == kForever)
    {
        return TimePoint::max();
    }

    return Clock::now() + duration;
}
} // namespace mk::util
