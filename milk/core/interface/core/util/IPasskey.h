#pragma once
#include <core/IMacro.h>

namespace mk::util
{
template<typename T>
class Passkey
{
private:
    friend T;
    explicit Passkey() = default;
};

} // namespace mk::util
