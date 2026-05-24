#pragma once

#include <core/IMacro.h>
#include <core/container/IVector.h>
#include <core/container/IString.h>
#include <core/util/IPasskey.h>
#include <core/IUniquePtr.h>
#include <fmt/format.h>
#include <source_location>
#include <core/log/ILogLevel.h>

namespace mk::log
{

// fwd declaration
class ISink;
class IFormatter;
struct LogContext;

using IFormatterPtr = UniquePtr<IFormatter>;
using ISinkPtr = UniquePtr<ISink>;
using LogCategory = StringView;
using MemoryBuffer = fmt::basic_memory_buffer<char, 1024>;

// Only support ASCII flags
struct FlagFormatterPair
{
    IFormatterPtr formatter;
    char          flag;
};

struct LogSystemConfig
{
    static constexpr StringView kDefaultPattern =
        "%N %Y-%M-%D %h:%m:%s:%e %2t [%c] %l: %v [%F:%-4#]";

    StringView                    pattern;
    VectorView<ISinkPtr>          sinks;
    VectorView<FlagFormatterPair> userFlagFormatters;
};

// TODO(Cheese_S): Make logging happens on its own thread
// TODO(Cheese_S): Make threads log their t id
class LogSystem
{
    MK_NON_MOVABLE_NON_COPYABLE(LogSystem);
    using LogSystemPasskey = util::Passkey<LogSystem>;

public:
    using FormatterVector = StackVector<IFormatterPtr, 8>;
    using SinkVector = StackVector<ISinkPtr, 4>;

    static Result makeLogSystem(LogSystemConfig config, UniquePtr<LogSystem>& outSystem);

    LogSystem(LogSystemPasskey,
              VectorView<IFormatterPtr> formatters,
              VectorView<ISinkPtr>      sinks,
              StringView                pattern);

    // eOk, if nothing went wrong
    // eInvalidParam if the pattern is deemd invalid (repeated / no formatter flags)
    // eInvalidParam if no sinks is passed in (repeated / no formatter flags)
    // eOutOfRange if padding is over 255.
    template<typename... ArgsType>
    void log(LogCategory                     category,
             LogLevel                        level,
             std::source_location            sourceLocation,
             fmt::format_string<ArgsType...> fmt,
             ArgsType&&... args);

private:
    void logImpl(LogCategory          category,
                 LogLevel             level,
                 std::source_location sourceLocation,
                 StringView           msg);

    SinkVector       sinks_;
    FormatterVector  formatters_;
    String           pattern_;
    std::atomic<u64> logCount_;
};

template<typename... ArgsType>
void LogSystem::log(LogCategory                     category,
                    LogLevel                        level,
                    std::source_location            sourceLocation,
                    fmt::format_string<ArgsType...> fmt,
                    ArgsType&&... args)
{
    MemoryBuffer formattedMsg;
    fmt::vformat_to(fmt::appender(formattedMsg), fmt, fmt::make_format_args(args...));
    logImpl(category,
            level,
            sourceLocation,
            StringView(formattedMsg.begin(), formattedMsg.size()));
}

#define MK_LOG_INFO(fmt, ...)                                                  \
    AppContext<mk::log::LogSystem>::get().log(kDefaultLogCategory,             \
                                              mk::log::LogLevel::eInfo,        \
                                              std::source_location::current(), \
                                              fmt,                             \
                                              __VA_ARGS__);
#define MK_LOG_DEBUG(fmt, ...)                                                 \
    AppContext<mk::log::LogSystem>::get().log(kDefaultLogCategory,             \
                                              mk::log::LogLevel::eDebug,       \
                                              std::source_location::current(), \
                                              fmt,                             \
                                              __VA_ARGS__);
#define MK_LOG_WARN(fmt, ...)                                                  \
    AppContext<mk::log::LogSystem>::get().log(kDefaultLogCategory,             \
                                              mk::log::LogLevel::eWarning,     \
                                              std::source_location::current(), \
                                              fmt,                             \
                                              __VA_ARGS__);
#define MK_LOG_ERROR(fmt, ...)                                                 \
    AppContext<mk::log::LogSystem>::get().log(kDefaultLogCategory,             \
                                              mk::log::LogLevel::eError,       \
                                              std::source_location::current(), \
                                              fmt,                             \
                                              __VA_ARGS__);

#define MK_CAT_LOG_INFO(category, fmt, ...)                                    \
    AppContext<mk::log::LogSystem>::get().log(k##category##LogCategory,        \
                                              mk::log::LogLevel::eInfo,        \
                                              std::source_location::current(), \
                                              fmt,                             \
                                              __VA_ARGS__);
#define MK_CAT_LOG_DEBUG(category, fmt, ...)                                   \
    AppContext<mk::log::LogSystem>::get().log(k##category##LogCategory,        \
                                              mk::log::LogLevel::eDebug,       \
                                              std::source_location::current(), \
                                              fmt,                             \
                                              __VA_ARGS__);
#define MK_CAT_LOG_WARN(category, fmt, ...)                                    \
    AppContext<mk::log::LogSystem>::get().log(k##category##LogCategory,        \
                                              mk::log::LogLevel::eWarning,     \
                                              std::source_location::current(), \
                                              fmt,                             \
                                              __VA_ARGS__);
#define MK_CAT_LOG_ERROR(category, fmt, ...)                                   \
    AppContext<mk::log::LogSystem>::get().log(k##category##LogCategory,        \
                                              mk::log::LogLevel::eError,       \
                                              std::source_location::current(), \
                                              fmt,                             \
                                              __VA_ARGS__);

#define MK_LOG_ERROR_AND_RETURN_IF_NOT_OK(res, fmt, ...)  \
    do                                                    \
    {                                                     \
        Result MK_CONCAT(x, __LINE__) = res;              \
        if (isNotOk(MK_CONCAT(x, __LINE__)))              \
        {                                                 \
            MK_LOG_ERROR(fmt __VA_OPT__(, ) __VA_ARGS__); \
            return MK_CONCAT(x, __LINE__);                \
        }                                                 \
    } while (0)

#define MK_LOG_ERROR_AND_RETURN_RET_IF_FALSE(x, ret, fmt, ...) \
    do                                                         \
    {                                                          \
        bool MK_CONCAT(b, __LINE__) = (x);                     \
        if (!(MK_CONCAT(b, __LINE__)))                         \
        {                                                      \
            MK_LOG_ERROR(fmt __VA_OPT__(, ) __VA_ARGS__);      \
            return ret;                                        \
        }                                                      \
    } while (0)

} // namespace mk::log
