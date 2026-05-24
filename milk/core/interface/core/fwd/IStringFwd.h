#pragma once
#include <core/fwd/IMemoryFwd.h>

namespace mk::details
{

template<typename Storage>
class IString;

}

namespace mk
{
using String = details::IString<mm::LinearHeapStorage<char>>;

template<usize N>
using StackString = details::IString<mm::LinearStackStorage<char, N + 1>>;

class StringView;
} // namespace mk
