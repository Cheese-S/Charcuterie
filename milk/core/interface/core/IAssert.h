#pragma once
#include <core/IResult.h>
#include <core/ICompileFlag.h>
#include <core/log/IRawLog.h>
#include <intrin.h>

#ifdef MK_DEBUG

    #define MK_ASSERT(x)                                   \
        do                                                 \
        {                                                  \
            bool b = (x);                                  \
            if (!b)                                        \
            {                                              \
                MK_RAW_LOG_ERROR("Assertion failed: " #x); \
                __debugbreak();                            \
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
                __debugbreak();                     \
                assert(b);                          \
            }                                       \
        } while (0)

#else
    #define MK_ASSERT(x)
    #define MK_ASSERTF(x, fmt, ...)

#endif

#define MK_ASSERT_UNREACHABLE()  MK_ASSERTF(false, "Unreachable");

#define MK_ASSERT_IS_OK(res)     MK_ASSERT(isOk(res))
#define MK_ASSERT_IS_NOT_OK(res) MK_ASSERT(isNotOk(res))
