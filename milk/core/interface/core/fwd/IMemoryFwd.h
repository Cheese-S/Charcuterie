#pragma once

#include <core/IType.h>

namespace mk::mm
{
template<typename T>
class LinearHeapStorage;

template<typename T, usize N>
class LinearStackStorage;

template<typename T, usize N, typename FallbackStorage>
class LinearStackStorageWithFallback;

template<typename T, usize N>
using LinerInlineStorage = LinearStackStorageWithFallback<T, N, LinearHeapStorage<T>>;

}; // namespace mk::mm
