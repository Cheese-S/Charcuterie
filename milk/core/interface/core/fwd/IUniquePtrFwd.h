#pragma once

namespace mk::mm
{
template<typename T>
class UniquePtr;

template<typename T>
class UniquePtr<T[]>;

} // namespace mk::mm
