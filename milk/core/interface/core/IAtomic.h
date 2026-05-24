#pragma once
#include <atomic>

namespace mk
{
template<typename T>
bool compareExchangeWeak(
    std::atomic<T>&   atomic,
    T                 expected,
    T                 desired,
    std::memory_order readOrder = std::memory_order::memory_order_seq_cst,
    std::memory_order writeOrder = std::memory_order::memory_order_seq_cst)
{
    return atomic.compare_exchange_weak(expected, desired, readOrder, writeOrder);
}
} // namespace mk
