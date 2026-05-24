#pragma once

namespace mk::mm::details
{
template<typename T, bool IsThreadSafe>
class ISharedPtr;

template<typename T, bool IsThreadSafe>
class IWeakPtr;

} // namespace mk::mm::details

namespace mk::mm
{
template<typename T>
using SharedPtr = details::ISharedPtr<T, true>;

template<typename T>
using StSharedPtr = details::ISharedPtr<T, false>;

template<typename T>
using WeakPtr = details::IWeakPtr<T, true>;

template<typename T>
using StWeakPtr = details::IWeakPtr<T, false>;

} // namespace mk::mm
