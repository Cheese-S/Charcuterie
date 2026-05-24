#pragma once

namespace mk::cc
{
template<typename T>
concept IsLockable = requires(T m) {
    m.lock();
    m.unlock();
};

template<typename T>
concept IsSharedLockable = requires(T m) {
    m.lockShared();
    m.unlockShared();
};
} // namespace mk::cc
