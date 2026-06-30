#pragma once
#include <core/container/IString.h>

namespace mk::log
{
using LogCategory = StringView;
}

namespace mk
{
inline constexpr log::LogCategory kJobSystemLogCategory = "JobSystem";
inline constexpr log::LogCategory kAssetLogCategory = "Asset";

#define MK_DEFINE_DEFAULT_LOG_CATEGORY(name)                              \
    namespace                                                             \
    {                                                                     \
    [[maybe_unused]] constexpr mk::log::LogCategory kDefaultLogCategory = \
        mk::k##name##LogCategory;                                         \
    } // namespace mk

#define MK_ADD_AND_DEFINE_LOG_CATEGORY(name, str)                     \
    namespace mk                                                      \
    {                                                                 \
    inline constexpr mk::log::LogCategory k##name##LogCategory = str; \
    }                                                                 \
    MK_DEFINE_DEFAULT_LOG_CATEGORY(name)
} // namespace mk
