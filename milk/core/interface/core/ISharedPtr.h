#include <type_traits>
#include <atomic>
#include <core/IType.h>
#include <core/IMacro.h>
#include <core/ITraits.h>
#include <core/IAssert.h>

namespace mk::details
{
// forward declare
template<typename T, bool IsThreadSafe>
class ISharedPtr;

template<typename T, bool IsThreadSafe>
class IWeakPtr;

template<bool IsThreadSafe>
class StrongReference;

template<bool IsThreadSafe>
class WeakReference;

// For some reasons, using std::contional_t to select types here causes .clangd to crash.
// TODO(Cheese_S): maybe fix it once clangd gets updated
template<bool>
struct RefControllerStorageType;

template<>
struct RefControllerStorageType<true>
{
    using type = std::atomic<u32>;
};

template<>
struct RefControllerStorageType<false>
{
    using type = u32;
};

template<bool IsThreadSafe>
class RefController
{
    using RefCountStorageType = RefControllerStorageType<IsThreadSafe>::type;
    MK_NON_COPYABLE(RefController);

public:
    RefController(): strongCount_(1), weakCount_(1) {}

    virtual ~RefController() = default;

    void addStrongRef()
    {
        if constexpr (IsThreadSafe)
        {
            strongCount_.fetch_add(1, std::memory_order_relaxed);
            return;
        }

        strongCount_++;
    }

    bool tryAddStrongRef()
    {
        if constexpr (IsThreadSafe)
        {
            u32 oldCount = strongCount_.load(std::memory_order_relaxed);
            while (true)
            {
                if (!oldCount)
                {
                    return false;
                }

                if (strongCount_.compare_exchange_weak(oldCount,
                                                       oldCount + 1,
                                                       std::memory_order_relaxed))
                {
                    break;
                }
            }
            MK_ASSERT(oldCount);
            return true;
        }

        if (!strongCount_)
        {
            return false;
        }

        strongCount_++;
        return true;
    }

    void addWeakRef()
    {
        if constexpr (IsThreadSafe)
        {
            weakCount_.fetch_add(1, std::memory_order_relaxed);
            return;
        }
        weakCount_++;
    }

    // ! Use for debug purpose only
    u32 getStrongRefCount()
    {
        if constexpr (IsThreadSafe)
        {
            return strongCount_.load(std::memory_order_relaxed);
        }
        else
        {
            return strongCount_;
        }
    }

    void releaseStrongRef()
    {
        if constexpr (IsThreadSafe)
        {
            // Use acq_rel here so that all modifications to the object are visible here.
            // We don't want to trigger something in the object's destructor before we
            // can confirm the count is 0
            u32 oldCount = strongCount_.fetch_sub(1, std::memory_order_acq_rel);
            if (oldCount == 1)
            {
                destructObject();
                releaseWeakRef();
            }

            return;
        }

        if (!(--strongCount_))
        {
            destructObject();
        }
    }

    void releaseWeakRef()
    {
        if constexpr (IsThreadSafe)
        {
            u32 oldCount = weakCount_.fetch_sub(1, std::memory_order_acq_rel);
            if (oldCount == 1)
            {
                delete this;
            }
            return;
        }

        if (!(--weakCount_))
        {
            delete this;
        }
    }

private:
    virtual void        destructObject() = 0;
    RefCountStorageType strongCount_;
    RefCountStorageType weakCount_;
};

template<bool IsThreadSafe>
class StrongReference
{
    using RefControllerType = RefController<IsThreadSafe>;

public:
    StrongReference(): controller_(nullptr) {}

    StrongReference(RefControllerType* controller): controller_(controller) {}

    ~StrongReference()
    {
        if (controller_)
        {
            controller_->releaseStrongRef();
        }
    }

    StrongReference(const StrongReference& other): controller_(other.controller_)
    {
        if (controller_)
        {
            controller_->addStrongRef();
        }
    }

    StrongReference(StrongReference&& other): controller_(other.controller_)
    {
        other.controller_ = nullptr;
    }

    StrongReference& operator=(const StrongReference& other)
    {
        if (this == &other)
        {
            return *this;
        }

        if (controller_)
        {
            controller_->releaseStrongRef();
        }

        if (other.controller_)
        {
            other.controller_->addStrongRef();
        }

        controller_ = other.controller_;
        return *this;
    }

    StrongReference& operator=(StrongReference&& other)
    {
        if (this == &other)
        {
            return *this;
        }

        if (controller_)
        {
            controller_->releaseStrongRef();
        }

        controller_ = other.controller_;
        other.controller_ = nullptr;

        return *this;
    }

    RefControllerType* getController()
    {
        return controller_;
    }

    u32 count()
    {
        if (!controller_)
        {
            return 0;
        }
        return controller_->getStrongRefCount();
    }

    bool isValid() const
    {
        return controller_ && controller_->getStrongRefCount() > 0;
    }

private:
    friend class WeakReference<IsThreadSafe>;

    RefControllerType* controller_;
};

template<bool IsThreadSafe>
class WeakReference
{
    using RefControllerType = RefController<IsThreadSafe>;

public:
    WeakReference(): controller_(nullptr) {}

    WeakReference(RefControllerType* controller): controller_(controller) {}

    WeakReference(const StrongReference<IsThreadSafe>& strongRef):
        controller_(strongRef.controller_)
    {
        if (controller_)
        {
            controller_->addWeakRef();
        }
    }

    ~WeakReference()
    {
        if (controller_)
        {
            controller_->releaseWeakRef();
        }
    }

    WeakReference(const WeakReference& other): controller_(other.controller_)
    {
        if (controller_)
        {
            controller_->addWeakRef();
        }
    }

    WeakReference(WeakReference&& other): controller_(other.controller_)
    {
        other.controller_ = nullptr;
    }

    WeakReference& operator=(const WeakReference& other)
    {
        if (this == &other)
        {
            return *this;
        }

        if (controller_)
        {
            controller_->releaseWeakRef();
        }

        if (other.controller_)
        {
            other.controller_->addWeakRef();
        }

        controller_ = other.controller_;
        return *this;
    }

    WeakReference& operator=(const StrongReference<IsThreadSafe>& other)
    {
        if (controller_)
        {
            controller_->releaseWeakRef();
        }

        if (other.controller_)
        {
            other.controller_->addWeakRef();
        }

        controller_ = other.controller_;
        return *this;
    }

    WeakReference& operator=(WeakReference&& other)
    {
        if (this == &other)
        {
            return *this;
        }

        if (controller_)
        {
            controller_->releaseWeakRef();
        }

        controller_ = other.controller_;
        other.controller_ = nullptr;

        return *this;
    }

    WeakReference& operator=(nullptr_t)
    {
        controller_ = nullptr;
        return *this;
    }

    RefControllerType* tryAcquireRefController()
    {
        if (!controller_ || !controller_->tryAddStrongRef())
        {
            return nullptr;
        }
        return controller_;
    }

    bool isValid() const
    {
        return controller_ && controller_->getStrongRefCount() > 0;
    }

private:
    RefControllerType* controller_;
};

// Object is allocated next to the refcount
template<typename T, bool IsThreadSafe>
class IntrusiveRefCount: public RefController<IsThreadSafe>
{
public:
    template<typename... ArgsType>
    explicit IntrusiveRefCount(ArgsType&&... args)
    {
        ::new (static_cast<void*>(storage_.bytes)) T(std::forward<ArgsType>(args)...);
    }

    void destructObject() override
    {
        getObjectPtr()->~T();
    }

    T* getObjectPtr()
    {
        return std::launder(reinterpret_cast<T*>(storage_.bytes));
    }

private:
    TypeCompatibleBytes<T> storage_;
};

// Object is passed in externally
template<typename T, bool IsThreadSafe>
class DefaultDeleteRefCount: public RefController<IsThreadSafe>
{
public:
    explicit DefaultDeleteRefCount(T* obj): obj_(obj) {};

    void destructObject() override
    {
        delete obj_;
    }

private:
    T* obj_;
};

template<typename T, bool IsThreadSafe>
ISharedPtr<T, IsThreadSafe> makeSharedPtr(RefController<IsThreadSafe>* refCount, T* obj)
{
    return ISharedPtr<T, IsThreadSafe>(refCount, obj);
}

template<typename T, bool IsThreadSafe = true>
class ISharedPtr
{
    static_assert(!(std::is_void_v<T>), "void type is not allowed for shared ptr.");

    template<typename U, bool OtherIsThreadSafe>
    friend class ISharedPtr;

    template<typename U, bool OtherIsThreadSafe>
    friend class IWeakPtr;

    using RefType = StrongReference<IsThreadSafe>;
    using RefControllerType = RefController<IsThreadSafe>;

public:
    ISharedPtr(nullptr_t): ptr_(nullptr) {}

    ISharedPtr(): ptr_(nullptr) {}

    // Assume the ptr is not owned by other smart pointers and allocated on heap.
    // Nullptr is a valid input.
    // Otherwise, it's UB.
    explicit ISharedPtr(T* ptr):
        ref_(new DefaultDeleteRefCount<T, IsThreadSafe>(ptr)), ptr_(ptr)
    {
    }

    ISharedPtr(const ISharedPtr& other): ref_(other.ref_), ptr_(other.ptr_) {}

    ISharedPtr& operator=(const ISharedPtr& other)
    {
        if (this == &other)
        {
            return *this;
        }

        ref_ = other.ref_;
        ptr_ = other.ptr_;
        return *this;
    }

    ISharedPtr& operator=(nullptr_t)
    {
        ref_ = nullptr;
        ptr_ = nullptr;
        return *this;
    }

    template<typename U, bool OtherIsThreadSafe>
        requires(std::is_convertible_v<U*, T*> && IsThreadSafe == OtherIsThreadSafe)
    ISharedPtr(const ISharedPtr<U, OtherIsThreadSafe>& other):
        ref_(other.ref_), ptr_(other.ptr_)
    {
    }

    template<typename U, bool OtherIsThreadSafe>
        requires(std::is_convertible_v<U*, T*> && IsThreadSafe == OtherIsThreadSafe)
    ISharedPtr(ISharedPtr<U, OtherIsThreadSafe>&& other):
        ref_(std::move(other.ref_)), ptr_(other.ptr_)
    {
        other.ptr_ = nullptr;
        other.ref_ = nullptr;
    }

    template<typename U, bool OtherIsThreadSafe>
        requires(std::is_convertible_v<U*, T*> && IsThreadSafe == OtherIsThreadSafe)
    ISharedPtr& operator=(const ISharedPtr<U, OtherIsThreadSafe>& other)
    {
        ref_ = other.ref_;
        ptr_ = other.ptr_;
        return *this;
    }

    template<typename U, bool OtherIsThreadSafe>
        requires(std::is_convertible_v<U*, T*> && IsThreadSafe == OtherIsThreadSafe)
    ISharedPtr& operator=(ISharedPtr<U, OtherIsThreadSafe>&& other) noexcept
    {
        ref_ = std::move(other.ref_);
        ptr_ = other.ptr_;

        other.ptr_ = nullptr;
        other.ref_ = nullptr;

        return *this;
    }

    // Should be only used during debugging
    u32 getCount()
    {
        return ref_.count();
    }

    T* operator->()
    {
        return ptr_;
    }

    T* get()
    {
        return ptr_;
    }

    T& operator*()
    {
        return *ptr_;
    }

    explicit operator bool() const
    {
        return ptr_;
    }

    void reset()
    {
        *this = ISharedPtr<T, IsThreadSafe>();
    }

    bool isValid() const
    {
        return ref_.isValid();
    }

private:
    friend ISharedPtr<T, IsThreadSafe>
    details::makeSharedPtr<T, IsThreadSafe>(RefControllerType* controller, T* obj);

    friend class IWeakPtr<T, IsThreadSafe>;

    ISharedPtr(RefControllerType* controller, T* ptr): ref_(controller), ptr_(ptr) {};

    RefType ref_;
    T*      ptr_;
};

template<typename T, bool IsThreadSafe = true>
class IWeakPtr
{
    static_assert(!(std::is_void_v<T>), "void type is not allowed for shared ptr.");

    template<typename U, bool OtherIsThreadSafe>
    friend class IWeakPtr;

    using RefType = WeakReference<IsThreadSafe>;
    using RefCountType = RefController<IsThreadSafe>;

public:
    IWeakPtr(nullptr_t): ptr_(nullptr) {}

    IWeakPtr(): ptr_(nullptr) {}

    IWeakPtr(const IWeakPtr& other): ref_(other.ref_), ptr_(other.ptr_) {}

    template<typename U, bool OtherIsThreadSafe>
        requires(std::is_convertible_v<U, T> && IsThreadSafe == OtherIsThreadSafe)
    IWeakPtr(const ISharedPtr<U, OtherIsThreadSafe>& other):
        ref_(other.ref_), ptr_(other.ptr_)
    {
    }

    IWeakPtr& operator=(const IWeakPtr& other)
    {
        if (this == &other)
        {
            return *this;
        }

        ref_ = other.ref_;
        ptr_ = other.ptr_;
        return *this;
    }

    IWeakPtr& operator=(nullptr_t)
    {
        ref_ = nullptr;
        ptr_ = nullptr;
        return *this;
    }

    template<typename U, bool OtherIsThreadSafe>
        requires(std::is_convertible_v<U, T> && IsThreadSafe == OtherIsThreadSafe)
    IWeakPtr(const IWeakPtr<U, OtherIsThreadSafe>& other):
        ref_(other.ref_), ptr_(other.ptr_)
    {
    }

    template<typename U, bool OtherIsThreadSafe>
        requires(std::is_convertible_v<U, T> && IsThreadSafe == OtherIsThreadSafe)
    IWeakPtr(IWeakPtr<U, OtherIsThreadSafe>&& other):
        ref_(std::move(other.ref_)), ptr_(other.ptr_)
    {
        other.ptr_ = nullptr;
    }

    template<typename U, bool OtherIsThreadSafe>
        requires(std::is_convertible_v<U, T> && IsThreadSafe == OtherIsThreadSafe)
    IWeakPtr& operator=(const IWeakPtr<U, OtherIsThreadSafe>& other)
    {
        if (this == &other)
        {
            return *this;
        }

        ref_ = other.ref_;
        ptr_ = other.ptr_;
        return *this;
    }

    template<typename U, bool OtherIsThreadSafe>
        requires(std::is_convertible_v<U, T> && IsThreadSafe == OtherIsThreadSafe)
    IWeakPtr& operator=(const ISharedPtr<U, OtherIsThreadSafe>& other)
    {
        ref_ = other.ref_;
        ptr_ = other.ptr_;
        return *this;
    }

    template<typename U, bool OtherIsThreadSafe>
        requires(std::is_convertible_v<U, T> && IsThreadSafe == OtherIsThreadSafe)
    IWeakPtr& operator=(IWeakPtr<U, OtherIsThreadSafe>&& other) noexcept
    {
        if (this == &other)
        {
            return *this;
        }

        ref_ = std::move(other.ref_);
        ptr_ = other.ptr_;

        other.ptr_ = nullptr;
        other.ref_ = nullptr;

        return *this;
    }

    explicit operator bool() const
    {
        return ptr_;
    }

    ISharedPtr<T, IsThreadSafe> lock()
    {
        if (!ptr_)
        {
            return nullptr;
        }

        auto* ref = ref_.tryAcquireRefController();
        if (!ref)
        {
            return nullptr;
        }
        return ISharedPtr<T, IsThreadSafe>(ref, ptr_);
    }

    bool isValid() const
    {
        return ref_.isValid();
    }

    void reset()
    {
        *this = IWeakPtr<T, IsThreadSafe>();
    }

private:
    RefType ref_;
    T*      ptr_;
};

} // namespace mk::details

namespace mk
{

template<typename T>
using SharedPtr = details::ISharedPtr<T, true>;

// Single threaded shared ptr
// Slightly more performant
template<typename T>
using StSharedPtr = details::ISharedPtr<T, false>;

template<typename T>
using WeakPtr = details::IWeakPtr<T, true>;

template<typename T>
using StWeakPtr = details::IWeakPtr<T, false>;

template<typename T, bool IsThreadSafe = true, typename... ArgsType>
details::ISharedPtr<T, IsThreadSafe> makeShared(ArgsType&&... args)
{
    details::IntrusiveRefCount<T, IsThreadSafe>* intrusiveRefCount =
        new details::IntrusiveRefCount<T, IsThreadSafe>(std::forward<ArgsType>(args)...);
    return details::makeSharedPtr(
        static_cast<details::RefController<IsThreadSafe>*>(intrusiveRefCount),
        intrusiveRefCount->getObjectPtr());
}

template<typename T, bool IsThreadSafe>
struct IsZeroInitializable<details::ISharedPtr<T, IsThreadSafe>>
{
    enum
    {
        value = true // NOLINT
    };
};

template<typename T, bool IsThreadSafe>
struct IsZeroInitializable<details::IWeakPtr<T, IsThreadSafe>>
{
    enum
    {
        value = true // NOLINT
    };
};

}; // namespace mk
