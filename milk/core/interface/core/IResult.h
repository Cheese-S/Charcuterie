#pragma once

#include <cassert>
#include <fmt/core.h>
#include <core/IMacro.h>

namespace mk
{

// CHECK asserts on cond

#define MK_RETURN_RET_IF_NOT_OK(x, ret)        \
    do                                         \
    {                                          \
        Result MK_CONCAT(res, __LINE__) = (x); \
        if (isNotOk(MK_CONCAT(res, __LINE__))) \
        {                                      \
            return (ret);                      \
        }                                      \
    } while (0)

#define MK_RETURN_IF_NOT_OK(x)                 \
    do                                         \
    {                                          \
        Result MK_CONCAT(res, __LINE__) = (x); \
        if (isNotOk(MK_CONCAT(res, __LINE__))) \
        {                                      \
            return MK_CONCAT(res, __LINE__);   \
        }                                      \
    } while (0)

enum class [[nodiscard]] Result
{
    eOk = 0,
    eInvalidParam,      // When the args / param passed in is not what the function expects
    eInvalidPermission, // When trying to open file without the correct access rights
    eOutOfCapactiy,     // When OOM / Running out capcity
    eDoesNotExist,      // When file does not exists
    eAlreadyExist,      // When file / directory already exist
    eNotInitialized,    // When the internal state of an object has not been initialized
    eUnexpected,        // When procedure unexpectedly failed
    eOutOfRange,        // When the value exceeds expected limit
    eNotFound,          // When the expected state cannot be found.
    eTimeout,           // When the operation timed out
    eUnknown,           // When something unkown happned. Limit the use of this.
};

constexpr bool isOk(Result result)
{
    return result == Result::eOk;
}

constexpr bool isNotOk(Result result)
{
    return result != Result::eOk;
}

// inline Result errnoToResult(errno_t err)
// {
//     switch (err)
//     {
//     case 0:
//         return Result::eOk;
//     case EINVAL:
//         return Result::eInvalidParam;
//     case EACCES:
//         return Result::eInvalidAccess;
//     case ENOENT:
//         return Result::eDoesNotExist;
//     default:
//         MK_ASSERT(false);
//         return Result::eUnknown;
//     }
// };

} // namespace mk
template<>
struct fmt::formatter<mk::Result>
{
    constexpr auto parse(fmt::format_parse_context& ctx)
    {
        return ctx.begin();
    }

    template<typename Context>
    auto format(mk::Result value, Context& ctx) const
    {
        switch (value)
        {
        case mk::Result::eOk:
            return fmt::format_to(ctx.out(), "Result::eOk");
        case mk::Result::eInvalidParam:
            return fmt::format_to(ctx.out(), "Result::eInvalidParam");
        case mk::Result::eInvalidPermission:
            return fmt::format_to(ctx.out(), "Result::eInvalidPermission");
        case mk::Result::eOutOfCapactiy:
            return fmt::format_to(ctx.out(), "Result::eOutOfCapactiy");
        case mk::Result::eDoesNotExist:
            return fmt::format_to(ctx.out(), "Result::eDoesNotExist");
        case mk::Result::eAlreadyExist:
            return fmt::format_to(ctx.out(), "Result::eAlreadyExist");
        case mk::Result::eNotInitialized:
            return fmt::format_to(ctx.out(), "Result::eNotInitialized");
        case mk::Result::eUnexpected:
            return fmt::format_to(ctx.out(), "Result::eUnexpected");
        case mk::Result::eOutOfRange:
            return fmt::format_to(ctx.out(), "Result::eOutOfRange");
        case mk::Result::eNotFound:
            return fmt::format_to(ctx.out(), "Result::eNotFound");
        case mk::Result::eTimeout:
            return fmt::format_to(ctx.out(), "Result::eTimeout");
        case mk::Result::eUnknown:
            return fmt::format_to(ctx.out(), "Result::eUnknown");
            break;
        }
    }
};
