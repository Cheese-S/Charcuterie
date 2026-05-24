#pragma once
#include <core/IType.h>
#include <type_traits>
#include <memory>

// NOLINTBEGIN
namespace mk
{
template<typename... Ts>
struct Or
{
    static constexpr bool value = (Ts::value || ...);
};

template<typename... Ts>
struct And
{
    static constexpr bool value = (Ts::value && ...);
};

template<typename T>
struct IsReferenceType
{
    enum
    {
        value = false
    };
};

template<typename T>
struct IsReferenceType<T&>
{
    enum
    {
        value = true
    };
};

template<typename T>
struct IsReferenceType<T&&>
{
    enum
    {
        value = true
    };
};

template<typename T>
struct IsPointerType
{
    enum
    {
        value = false
    };
};

template<typename T>
struct IsPointerType<T*>
{
    enum
    {
        value = true
    };
};

template<typename T>
struct IsPointerType<const T>
{
    enum
    {
        value = IsPointerType<T>::value
    };
};

template<typename T>
struct IsArithmeticType
{
    enum
    {
        value = false
    };
};

// clang-format off
template<> struct IsArithmeticType<float> { enum { value = true }; }; //NOLINT
template<> struct IsArithmeticType<double> { enum { value = true }; }; //NOLINT
template<> struct IsArithmeticType<long double> { enum { value = true }; }; //NOLINT
template<> struct IsArithmeticType<u8> { enum { value = true }; }; //NOLINT
template<> struct IsArithmeticType<u16> { enum { value = true }; }; //NOLINT
template<> struct IsArithmeticType<u32> { enum { value = true }; }; //NOLINT
template<> struct IsArithmeticType<u64> { enum { value = true }; }; //NOLINT
template<> struct IsArithmeticType<i8> { enum { value = true }; }; //NOLINT
template<> struct IsArithmeticType<i16> { enum { value = true }; }; //NOLINT
template<> struct IsArithmeticType<i32> { enum { value = true }; }; //NOLINT
template<> struct IsArithmeticType<i64> { enum { value = true }; }; //NOLINT
                                                                    

template<typename T> struct IsArithmeticType<const T> { enum { value = IsArithmeticType<T>::value }; }; //NOLINT
// clang-format on

template<typename T>
struct IsEnumType
{
    enum
    {
        value = std::is_enum<T>::value
    };
};

template<typename T>
struct IsBitwiseConstrutable
{
    static_assert(!IsReferenceType<T>::value,
                  "IsBitwiseConstrutable does not take reference type");

    enum
    {
        value = std::is_trivially_copy_constructible_v<T>
    };
};

template<typename T>
struct IsTriviallyDestructible
{
    enum
    {
        value = std::is_trivially_destructible_v<T>
    };
};

template<typename T>
struct IsZeroInitializable
{
    enum
    {
        value = Or<IsEnumType<T>, IsArithmeticType<T>, IsPointerType<T>>::value
    };
};

template<typename T>
struct IsZeroInitializable<std::unique_ptr<T>>
{
    enum
    {
        value = true
    };
};

template<typename T>
struct TypeCompatibleBytes
{
    alignas(T) std::byte bytes[sizeof(T)];
};

template<typename T, usize N>
struct TypeCompatibleBytesArray
{
    alignas(T) std::byte bytes[sizeof(T) * N];
};
// NOLINTEND
} // namespace mk
