#pragma once
#include <core/IMacro.h>
#include <core/concurrency/ILockTraits.h>

namespace mk::cc
{

template<typename T>
    requires IsLockable<T>
class ScopedLock
{
public:
    MK_NON_MOVABLE_NON_COPYABLE(ScopedLock);

    ScopedLock(T& lock): lock_(lock)
    {
        lock_.lock();
    };

    ~ScopedLock()
    {
        lock_.unlock();
    }

private:
    T& lock_;
};

template<typename T>
    requires IsSharedLockable<T>
class ScopedShareLock
{
public:
    MK_NON_MOVABLE_NON_COPYABLE(ScopedShareLock);

    ScopedShareLock(T& lock): lock_(lock)
    {
        lock_.lockShared();
    };

    ~ScopedShareLock()
    {
        lock_.unlockShared();
    }

private:
    T& lock_;
};
} // namespace mk::cc
