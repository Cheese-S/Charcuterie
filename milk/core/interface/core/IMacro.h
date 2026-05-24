#pragma once

#define MK_CONCAT_HELPER(x, y) x##y
#define MK_CONCAT(x, y)        MK_CONCAT_HELPER(x, y)

#define MK_UNREF(x)            ((void)(x))
#define MK_IGNORE_RET_VAL      (void)
#define MK_CARR_SIZE(arr)      ((sizeof(arr)) / sizeof((arr)[0]))

#define MK_DEFAULT_MOVABLE(type)  \
    type(type&& other) = default; \
    type& operator=(type&& other) = default

#define MK_DEFAULT_COPYABLE(type)      \
    type(const type& other) = default; \
    type& operator=(const type& other) = default

#define MK_NON_MOVABLE(type)     \
    type(type&& other) = delete; \
    type& operator=(type&& other) = delete

#define MK_NON_COPYABLE(type)         \
    type(const type& other) = delete; \
    type& operator=(const type& other) = delete

#define MK_NON_MOVABLE_NON_COPYABLE(type) \
    MK_NON_COPYABLE(type);                \
    MK_NON_MOVABLE(type)

#define MK_NON_MOVEABLE_DEFAULT_COPYABLE(type) \
    MK_DEFAULT_MOVABLE(type);                  \
    MK_NON_COPYABLE(type)

#define MK_DEFAULT_MOVABLE_DEFAULT_COPYABLE(type) \
    MK_DEFAULT_COPYABLE(type);                    \
    MK_DEFAULT_MOVABLE(type)

#define MK_DEFAULT_MOVEABLE_NON_COPYABLE(type) \
    MK_DEFAULT_MOVABLE(type);                  \
    MK_NON_COPYABLE(type)

// ------------------ Compile Configs ----------------------
#ifndef NDEBUG
    #define MK_DEBUG
#endif
