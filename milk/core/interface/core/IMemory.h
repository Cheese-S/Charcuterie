#pragma once

#include <core/IType.h>
#include <core/ITraits.h>
#include <core/IResult.h>
#include <core/IAssert.h>
#include <cstring>
#include <mimalloc.h>

namespace mk::mm::details
{
const usize kDefaultAlignment = 16;
template<typename T>
constexpr T align(T size, T alignment)
{
    return (((size) + (alignment)-1) & ~((alignment)-1));
}

void forceMiMallocLinkOrder();

} // namespace mk::mm::details

namespace mk::mm
{

#define FORCE_MIMALLOC_LINK_ORDER mi_version();

[[nodiscard]] void* alloc(usize requestSize,
                          usize alignment = details::kDefaultAlignment);

[[nodiscard]] void*
realloc(void* oldPtr, usize newSize, usize alignment = details::kDefaultAlignment);

void free(void* ptr);

i64 getMemSize();

template<typename T>
class LinearHeapStorage
{
public:
    [[nodiscard]] T* resize(T* storage, usize newCapacity)
    {
        return static_cast<T*>(mm::realloc(storage, newCapacity * sizeof(T)));
    }

    void free(T* storage)
    {
        mk::mm::free(storage);
    }

private:
};

template<typename T, usize N>
class LinearStackStorage
{
public:
    static constexpr usize kCapcity = N;

    constexpr T* resize([[maybe_unused]] T* storage, [[maybe_unused]] usize newCapacity)
    {
        MK_ASSERT(newCapacity <= N);
        return std::launder(reinterpret_cast<T*>(storage_.bytes));
    }

    void free([[maybe_unused]] T* storage)
    {
        // NO OP
    }

private:
    TypeCompatibleBytesArray<T, N> storage_;
};

template<typename T, usize N, typename FallbackStorage>
class LinearStackStorageWithFallback
{
public:
    LinearStackStorageWithFallback() = default;

    static constexpr usize kCapcity = N;

    T* resize(T* storage, usize newCapacity)
    {
        if (usingFallback_)
        {
            return fallback_.resize(storage, newCapacity);
        }

        if (newCapacity <= N)
        {
            return std::launder(reinterpret_cast<T*>(storage_.bytes));
        }

        T* fallbackStorage = fallback_.resize(nullptr, newCapacity);
        if (!usingFallback_)
        {
            std::memcpy(fallbackStorage, storage_.bytes, N * sizeof(T));
            usingFallback_ = true;
        }
        return fallbackStorage;
    }

    void free(T* storage)
    {
        if (usingFallback_)
        {
            fallback_.free(storage);
        }
    }

private:
    TypeCompatibleBytesArray<T, N> storage_;
    FallbackStorage                fallback_;
    bool                           usingFallback_;
};

template<typename T, usize N>
using LinerInlineStorage = LinearStackStorageWithFallback<T, N, LinearHeapStorage<T>>;

template<typename Storage, typename OtherStorage>
struct CanMoveBetweenStorage
{
    enum
    {
        value = false // NOLINT
    };
};

template<typename T>
struct CanMoveBetweenStorage<LinearHeapStorage<T>, LinearHeapStorage<T>>
{
    enum
    {
        value = true // NOLINT
    };
};

} // namespace mk::mm
