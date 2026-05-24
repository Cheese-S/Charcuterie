#pragma once

#include <core/ITraits.h>
#include <core/IMacro.h>
#include <core/concurrency/ISharedMutex.h>

namespace mk::cc::details
{
// TODO(Cheese_S): maybe unify this with IScopedLock
template<typename Mutex>
class ReadLockPolicy
{
public:
    using MutexType = Mutex;

    static void lock(Mutex& mutex)
    {
        mutex.lockShared();
    }

    static void unlock(Mutex& mutex)
    {
        mutex.unlockShared();
    }
};

template<typename Mutex>
class WriteLockPolicy
{
public:
    using MutexType = Mutex;

    static void lock(Mutex& mutex)
    {
        mutex.lock();
    }

    static void unlock(Mutex& mutex)
    {
        mutex.unlock();
    }
};

template<typename T, typename LockPolicy>
class LockedPtr
{
public:
    MK_NON_COPYABLE(LockedPtr);

    LockedPtr(T* ptr, LockPolicy::MutexType& mutex): ptr_(ptr), mutex_(&mutex)
    {
        LockPolicy::lock(*mutex_);
    }

    ~LockedPtr()
    {
        if (mutex_)
        {
            LockPolicy::unlock(*mutex_);
        }
    }

    LockedPtr(LockedPtr&& other): ptr_(other.ptr_), mutex_(other.mutex_) {}

    LockedPtr& operator=(LockedPtr&& other)
    {
        if (mutex_)
        {
            LockPolicy::unlock(*mutex_);
        }
        ptr_ = other.ptr_;
        mutex_ = other.mutex_;
    }

    T* operator->() const
    {
        return ptr_;
    }

    T& operator*() const
    {
        return *ptr_;
    }

private:
    T*                     ptr_;
    LockPolicy::MutexType* mutex_;
};

} // namespace mk::cc::details

namespace mk::cc
{
template<typename T, typename MutexType = SharedMutex>
class Sync
{
    using WLockedPtr = details::LockedPtr<T, details::WriteLockPolicy<MutexType>>;
    using RLockedPtr = details::LockedPtr<const T, details::ReadLockPolicy<MutexType>>;

public:
    MK_NON_MOVABLE_NON_COPYABLE(Sync);
    template<typename... ArgTypes>
    Sync(ArgTypes&&... args): datum_(std::forward<ArgTypes>(args)...)
    {
    }

    WLockedPtr wlock()
    {
        return WLockedPtr(&datum_, mutex_);
    }

    RLockedPtr rlock() const
    {
        return RLockedPtr(&datum_, mutex_);
    }

    template<typename ReadFn>
    void withRlock(ReadFn fn) const
    {
        static_assert(std::is_invocable_v<ReadFn, const T&>,
                      "ReadFn must accept const T& as a parameter");
        fn(*rlock());
    }

    template<typename WriteFn>
    void withWlock(WriteFn fn)
    {
        static_assert(std::is_invocable_v<WriteFn, T&>,
                      "ReadFn must accept T& as a parameter");
        fn(*wlock());
    }

private:
    T                 datum_;
    mutable MutexType mutex_;
};
} // namespace mk::cc
