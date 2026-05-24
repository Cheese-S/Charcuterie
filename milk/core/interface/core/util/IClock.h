#pragma once
#include <chrono>

namespace mk::util
{

using Clock = std::chrono::steady_clock;
using Duration = Clock::duration;
using TimePoint = Clock::time_point;

constexpr Duration kImmediate = Duration::zero();
constexpr Duration kForever = Duration::max();

TimePoint toTimePoint(Duration duration);

} // namespace mk::util
