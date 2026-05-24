#pragma once

#include <cassert>
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
    eInvalidParam, // When the args / param passed in is not what the function expects
    eInvalidPermission, // When trying to open file without the correct access rights
    eOutOfCapactiy,     // When OOM / Running out capcity
    eDoesNotExist,      // When file does not exists
    eNotInitialized,    // When the internal state of an object has not been initialized
    eUnexpected,        // When procedure unexpectedly failed
    eOutOfRange,        // When the value exceeds expected limit
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
