#pragma once
#include <core/fwd/IMemoryFwd.h>

namespace mk
{

template<typename T, typename Storage>
class IVector;

template<typename T>
using Vector = IVector<T, mm::LinearHeapStorage<T>>;

template<typename T, usize N>
using FixedVector = IVector<T, mm::LinearStackStorage<T, N>>;

template<typename T, usize N>
using StackVector = IVector<T, mm::LinearStackStorageWithFallback<T, N, mm::LinearHeapStorage<T>>>;

template<typename T>
class VectorView;

} // namespace mk
