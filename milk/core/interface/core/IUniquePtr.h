#pragma once
#include <core/IMacro.h>
#include <core/IType.h>
#include <core/ITraits.h>

namespace mk
{

template<typename T>
class DefaultDeleter
{
public:
    void operator()(T* ptr)
    {
        delete ptr;
    }
};

template<typename T>
class DefaultDeleter<T[]>
{
public:
    void operator()(T* ptr)
    {
        delete[] ptr;
    }
};

template<typename T>
class UniquePtr: private DefaultDeleter<T>
{
    template<typename U>
    friend class UniquePtr;

public:
    MK_NON_COPYABLE(UniquePtr);

    UniquePtr(): DefaultDeleter<T>(), ptr_(nullptr) {};

    UniquePtr(UniquePtr&& other) noexcept: ptr_(other.ptr_) // NOLINT
    {
        other.ptr_ = nullptr;
    }

    template<typename U>
        requires(std::is_convertible_v<U*, T*> && !std::is_array_v<U>)
    UniquePtr(UniquePtr<U>&& other) noexcept: ptr_(other.ptr_) // NOLINT
    {
        other.ptr_ = nullptr;
    }

    template<typename U>
        requires(std::is_convertible_v<U*, T*> && !std::is_array_v<U>)
    explicit UniquePtr(U* ptr): ptr_(ptr){};

    UniquePtr(nullptr_t): ptr_(nullptr) {}; // NOLINT

    template<typename U>
        requires(std::is_convertible_v<U*, T*> && !std::is_array_v<U>)
    UniquePtr& operator=(UniquePtr<U>&& other) noexcept
    {
        // Can't be the same pointer since T and U are different.
        // No self reference check
        T* prevPtr = ptr_;
        ptr_ = other.ptr_;
        other.ptr_ = nullptr;
        getDeleter()(prevPtr);
        return *this;
    }

    UniquePtr& operator=(UniquePtr&& other) noexcept
    {
        if (this != &other)
        {
            T* prevPtr = ptr_;
            ptr_ = other.ptr_;
            other.ptr_ = nullptr;
            getDeleter()(prevPtr);
        }
        return *this;
    }

    UniquePtr& operator=(nullptr_t)
    {
        T* prevPtr = ptr_;
        ptr_ = nullptr;
        getDeleter()(prevPtr);

        return *this;
    }

    ~UniquePtr()
    {
        getDeleter()(ptr_);
    }

    T* operator->() const
    {
        return ptr_;
    }

    T& operator*() const
    {
        return *ptr_;
    }

    explicit operator bool() const
    {
        return ptr_ != nullptr;
    }

    DefaultDeleter<T>& getDeleter()
    {
        return *static_cast<DefaultDeleter<T>*>(this);
    }

    void reset(T* ptr = nullptr)
    {
        if (ptr != ptr_)
        {
            T* prevPtr = ptr_;
            ptr_ = ptr;
            getDeleter()(prevPtr);
        }
    }

    T* get() const
    {
        return ptr_;
    }

    T* release()
    {
        T* prevPtr = ptr_;
        ptr_ = nullptr;
        return prevPtr;
    }

private:
    T* ptr_;
};

template<typename T>
class UniquePtr<T[]>: DefaultDeleter<T[]>
{
    template<typename U>
    friend class UniquePtr;

public:
    MK_NON_COPYABLE(UniquePtr);

    UniquePtr(): ptr_(nullptr) {};

    template<typename U>
        requires std::is_convertible_v<U (*)[], T (*)[]>
    UniquePtr(U* ptr): ptr_(ptr){}; // NOLINT

    template<typename U>
        requires std::is_convertible_v<U (*)[], T (*)[]>
    UniquePtr(UniquePtr<U[]>&& other) noexcept: ptr_(other.ptr_) // NOLINT
    {
        other.ptr_ = nullptr;
    }

    UniquePtr(nullptr_t): ptr_(nullptr) {}; // NOLINT, allow implcit construction

    template<typename U>
        requires std::is_convertible_v<U (*)[], T (*)[]>
    UniquePtr& operator=(UniquePtr<U[]>&& other) noexcept
    {
        T* prevPtr = ptr_;
        ptr_ = other.ptr_;
        other.ptr_ = nullptr;
        getDeleter()(prevPtr);

        return *this;
    }

    UniquePtr& operator=(nullptr_t)
    {
        T* prevPtr = ptr_;
        ptr_ = nullptr;
        getDeleter()(prevPtr);

        return *this;
    }

    ~UniquePtr()
    {
        getDeleter()(ptr_);
    }

    T& operator[](usize idx) const
    {
        return ptr_[idx];
    }

    explicit operator bool() const
    {
        return ptr_ != nullptr;
    }

    DefaultDeleter<T[]>& getDeleter()
    {
        return *static_cast<DefaultDeleter<T[]>*>(this);
    }

    void reset(T* ptr = nullptr)
    {
        if (ptr != ptr_)
        {
            T* prevPtr = ptr_;
            ptr_ = ptr;
            getDeleter()(prevPtr);
        }
    }

    T* get() const
    {
        return ptr_;
    }

    T* release()
    {
        T* prevPtr = ptr_;
        ptr_ = nullptr;
        return prevPtr;
    }

private:
    T* ptr_;
};

template<typename T, typename... ArgsType>
    requires(!std::is_array_v<T>)
UniquePtr<T> makeUnique(ArgsType&&... args)
{
    return UniquePtr<T>(new T(std::forward<ArgsType>(args)...));
}

template<typename T, typename... ArgsType>
    requires(std::is_array_v<T>)
UniquePtr<T> makeUnique(usize size)
{
    using type = std::remove_extent_t<T>;
    return UniquePtr<T>(new type[size]);
}

template<typename T>
struct IsZeroInitializable<UniquePtr<T>>
{
    enum
    {
        value = true // NOLINT
    };
};
}; // namespace mk
