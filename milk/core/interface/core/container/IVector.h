#pragma once
#include <algorithm>
#include <cstring>
#include <initializer_list>

#include <core/IResult.h>
#include <core/ITraits.h>
#include <core/IType.h>
#include <core/IMemory.h>
#include <core/IAssert.h>

namespace mk
{

// Don't use this type directly, use the aliasing listed below
template<typename T, typename Storage>
class IVector
{
public:
    using iterator = T*;
    using const_iterator = T*;

    IVector() = default;

    ~IVector()
    {
        destructElems(begin_, size());
        storage_.free(begin_);
    }

    IVector(std::initializer_list<T> il)
    {
        copyFrom(il.begin(), il.size());
    }

    IVector(const IVector& other)
    {
        copyFrom(other.cbegin(), other.size());
    }

    IVector(IVector&& other) noexcept
    {
        moveOrCopyFrom(std::move(other));
    }

    template<typename OtherStorage>
    IVector(const IVector<T, OtherStorage>& other) // NOLINT
    {
        copyFrom(other.cbegin(), other.size());
    }

    template<typename OtherStorage>
    IVector(IVector<T, OtherStorage>&& other) // NOLINT
    {
        destructElems(begin_, size());
        moveOrCopyFrom(std::move(other));
    }

    // This is needed to avoid constructor being called again in the following case
    // mk::Vector<int> vec = {1};   <- calls constructor
    // vec = {2};                   <- calls operator=
    IVector& operator=(std::initializer_list<T> il)
    {
        destructElems(begin_, size());
        copyFrom(il.begin(), il.size());
        return *this;
    }

    IVector& operator=(const IVector& other) noexcept
    {
        if (this != &other)
        {
            destructElems(begin_, size());
            copyFrom(other.cbegin(), other.size());
        }
        return *this;
    }

    IVector& operator=(IVector&& other) noexcept
    {
        if (this != &other)
        {
            destructElems(begin_, size());
            moveOrCopyFrom(std::move(other));
        }
        return *this;
    }

    template<typename OtherStorage>
    IVector& operator=(const IVector<T, OtherStorage>& other) noexcept
    {
        destructElems(begin_, size());
        copyFrom(other.cbegin(), other.size());
        return *this;
    }

    template<typename OtherStorage>
    IVector& operator=(IVector<T, OtherStorage>&& other) noexcept
    {
        destructElems(begin_, size());
        moveOrCopyFrom(std::move(other));
        return *this;
    }

    T& operator[](int index)
    {
        MK_ASSERT((0 <= index) && ((usize)index < capacity()));
        return begin_[index];
    }

    const T& operator[](int index) const
    {
        MK_ASSERT((0 <= index) && ((usize)index < capacity()));
        return begin_[index];
    }

    iterator begin() noexcept
    {
        return begin_;
    }

    iterator end() noexcept
    {
        return end_;
    }

    const_iterator cbegin() const noexcept
    {
        return begin_;
    }

    const_iterator cend() const noexcept
    {
        return end_;
    }

    T& push(const T& v)
    {
        return emplace(v);
    }

    T& push(T&& v)
    {
        return emplace(std::move(v));
    }

    T pop()
    {
        MK_ASSERT(size() > 0);
        T result = std::move(*(--end_));
        destructElems(end_, 1);
        return result;
    }

    template<typename... ArgsType>
    T& emplace(ArgsType&&... args)
    {
        ensureCapcity(size() + 1);
        ::new (static_cast<void*>(end_)) T(std::forward<ArgsType>(args)...);
        return *end_++;
    }

    void reserve(usize count)
    {
        ensureCapcity(count);
    }

    void resize(usize count)
    {
        ensureCapcity(count);

        usize prevCount = size();

        if (count < prevCount)
        {
            destructElems(end_, prevCount - count);
        }

        if (count > prevCount)
        {
            defaultConstructElems(end_, count - prevCount);
        }

        end_ = begin_ + count;
    }

    void clear()
    {
        destructElems(begin_, size());
        end_ = begin_;
    }

    T* data()
    {
        return begin_;
    }

    const T* cdata() const
    {
        return begin_;
    }

    [[nodiscard]] usize size() const
    {
        return end_ - begin_;
    }

    [[nodiscard]] bool empty() const
    {
        return !size();
    }

    [[nodiscard]] usize capacity() const
    {
        return capacity_ - begin_;
    }

private:
    void ensureCapcity(usize requestedCapacity)
    {
        usize currCapacity = capacity();
        if (requestedCapacity <= capacity())
        {
            return;
        }

        currCapacity *= 1.5;

        currCapacity = std::max(requestedCapacity, currCapacity);

        usize currSize = size();
        begin_ = storage_.resize(begin_, currCapacity);
        end_ = begin_ + currSize;
        capacity_ = begin_ + currCapacity;
    }

    template<typename OtherStorage>
    void moveOrCopyFrom(IVector<T, OtherStorage>&& other)
    {
        if constexpr (mm::CanMoveBetweenStorage<Storage, OtherStorage>::value)
        {
            storage_.free(begin_);

            begin_ = other.begin_;
            end_ = other.end_;
            capacity_ = other.capacity_;

            other.begin_ = nullptr;
            other.end_ = nullptr;
            other.capacity_ = nullptr;
        }
        else
        {
            copyFrom(other.begin(), other.size());
        }
    }

    void copyFrom(const T* src, usize count)
    {
        ensureCapcity(count);
        end_ = begin_ + count;
        if constexpr (IsBitwiseConstrutable<T>::value)
        {
            memcpy(begin(), src, count * sizeof(T));
            return;
        }
        constructElems(src, count);
    }

    void destructElems(T* begin, usize count)
    {
        if constexpr (!IsTriviallyDestructible<T>::value)
        {
            while (count)
            {
                begin->~T();
                begin++;
                count--;
            }
        }
    }

    void constructElems(const T* otherData, usize count)
    {
        T* it = begin();
        while (count)
        {
            ::new (static_cast<void*>(it)) T(*otherData);
            count--;
            it++;
        }
    }

    void defaultConstructElems(T* begin, usize count)
    {
        if constexpr (IsZeroInitializable<T>::value)
        {
            std::memset(begin, 0, sizeof(T) * count);
            return;
        }

        MK_ASSERT(count >= 0);
        while (count)
        {
            ::new (static_cast<void*>(begin)) T;
            count--;
            begin++;
        }
    }

    Storage storage_;
    T*      begin_ = nullptr;
    T*      end_ = nullptr;
    T*      capacity_ = nullptr;
};

template<typename T>
using Vector = IVector<T, mm::LinearHeapStorage<T>>;

template<typename T, usize N>
using FixedVector = IVector<T, mm::LinearStackStorage<T, N>>;

template<typename T, usize N>
using StackVector = IVector<T, mm::LinerInlineStorage<T, N>>;

template<typename T>
class VectorView
{
public:
    VectorView(): begin_(nullptr), end_(nullptr) {};

    template<typename Storage>
    VectorView(IVector<T, Storage>& vector): begin_(vector.begin()), end_(vector.end()){};

    VectorView(T* carr, usize size): begin_(carr), end_(carr + size) {}

    VectorView(T* begin, T* end): begin_(begin), end_(end) {}

    template<usize N>
    VectorView(T (&arr)[N]): begin_(arr), end_(arr + N)
    {
    }

    const T& operator[](int i) const
    {
        MK_ASSERT(i < end_ - begin_);
        return begin_[i];
    }

    T& operator[](int i)
    {
        MK_ASSERT(i < end_ - begin_);
        return begin_[i];
    }

    const T* cdata() const
    {
        return begin_;
    }

    const T* data()
    {
        return begin_;
    }

    const T* cbegin() const
    {
        return begin_;
    }

    const T* cend() const
    {
        return end_;
    }

    T* begin()
    {
        return begin_;
    }

    T* end()
    {
        return end_;
    }

    usize size() const
    {
        return end_ - begin_;
    }

    [[nodiscard]] bool empty() const
    {
        return !size();
    }

private:
    T* begin_;
    T* end_;
};

} // namespace mk
//
