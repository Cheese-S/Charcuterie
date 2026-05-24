#pragma once
#include <core/ITraits.h>
#include <type_traits>

namespace mk::enum_utils
{

template<typename E>
    requires(static_cast<bool>(IsEnumType<E>::value))
constexpr std::underlying_type_t<E> toUnderlying(E val)
{
    return static_cast<std::underlying_type_t<E>>(val);
}

template<typename E, typename... Es>
    requires(IsEnumType<E>::value && (std::is_same_v<E, Es> && ...))
constexpr E unionFlags(E lhs, Es... args)
{
    using T = std::underlying_type_t<E>;
    return static_cast<E>((static_cast<T>(lhs) | ... | static_cast<T>(args)));
}

template<typename E, typename... Es>
    requires(IsEnumType<E>::value && (std::is_same_v<E, Es> && ...))
constexpr bool hasFlags(E flags, Es... args)
{
    using T = std::underlying_type_t<E>;
    return ((static_cast<T>(flags) & static_cast<T>(args)) && ...);
}

template<typename E, typename... Es>
    requires(IsEnumType<E>::value && (std::is_same_v<E, Es> && ...))
constexpr E removeFlags(E flags, Es... args)
{
    using T = std::underlying_type_t<E>;
    return static_cast<E>((static_cast<T>(flags) & ... & ~static_cast<T>(args)));
}

} // namespace mk::enum_utils
