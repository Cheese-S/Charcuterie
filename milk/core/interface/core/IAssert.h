#pragma once
#include <core/IResult.h>
#include <core/log/IRawLog.h>

#define MK_ASSERT(x)                                   \
    do                                                 \
    {                                                  \
        bool b = (x);                                  \
        if (!b)                                        \
        {                                              \
            MK_RAW_LOG_ERROR("Assertion failed: " #x); \
            assert(b);                                 \
        }                                              \
                                                       \
    } while (0)

#define MK_ASSERTF(x, fmt, ...)                 \
    do                                          \
    {                                           \
        bool b = (x);                           \
        if (!b)                                 \
        {                                       \
            MK_RAW_LOG_ERROR(fmt, __VA_ARGS__); \
            assert(b);                          \
        }                                       \
    } while (0)

#define MK_ASSERT_UNREACHABLE()  MK_ASSERTF(false, "Unreachable");

#define MK_ASSERT_IS_OK(res)     MK_ASSERT(isOk(res))
#define MK_ASSERT_IS_NOT_OK(res) MK_ASSERT(isNotOk(res))
