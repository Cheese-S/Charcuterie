#pragma once
#include <source_location>
#include <fmt/format.h>
#include <core/fwd/IStringFwd.h>
#include <core/log/ILogLevel.h>

namespace mk::log::details
{
void rawLogImpl(LogLevel level, std::source_location sourceLocation, const char* view);
}

namespace mk::log
{

template<typename... ArgsType>
void rawLog(LogLevel                        level,
            std::source_location            sourceLocation,
            fmt::format_string<ArgsType...> fmt,
            ArgsType&&... args)
{
    fmt::basic_memory_buffer<char, 1024> formattedMsg;
    fmt::vformat_to(fmt::appender(formattedMsg), fmt, fmt::make_format_args(args...));
    formattedMsg.push_back('\0');
    details::rawLogImpl(level, sourceLocation, formattedMsg.begin());
}

#define MK_RAW_LOG_INFO(fmt, ...) \
    rawLog(mk::log::LogLevel::eInfo, std::source_location::current(), fmt, __VA_ARGS__)

#define MK_RAW_LOG_DEBUG(fmt, ...) \
    rawLog(mk::log::LogLevel::eDebug, std::source_location::current(), fmt, __VA_ARGS__)

#define MK_RAW_LOG_WARN(fmt, ...) \
    rawLog(mk::log::LogLevel::eWarning, std::source_location::current(), fmt, __VA_ARGS__)

#define MK_RAW_LOG_ERROR(fmt, ...) \
    rawLog(mk::log::LogLevel::eError, std::source_location::current(), fmt, __VA_ARGS__)

} // namespace mk::log
