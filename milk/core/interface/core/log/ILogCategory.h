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

#define MK_DEFINE_DEFAULT_LOG_CATEGORY(category)                          \
    namespace                                                             \
    {                                                                     \
    [[maybe_unused]] constexpr mk::log::LogCategory kDefaultLogCategory = \
        mk::k##category##LogCategory;                                     \
    } // namespace mk
} // namespace mk
