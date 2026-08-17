
#ifndef MK_VECTOR_IMPL
    #include <core/container/IVector.h>
#endif

namespace mk
{
// ---------------------------------- Vector ----------------------------------
template<typename T, typename Storage>
IVector<T, Storage>::IVector() = default;

template<typename T, typename Storage>
IVector<T, Storage>::~IVector()
{
    destructElems(begin_, size());
    storage_.free(begin_);
}

template<typename T, typename Storage>
IVector<T, Storage>::IVector(std::initializer_list<T> il)
{
    copyFrom(il.begin(), il.size());
}

template<typename T, typename Storage>
IVector<T, Storage>::IVector(const IVector& other)
{
    copyFrom(other.cbegin(), other.size());
}

template<typename T, typename Storage>
IVector<T, Storage>::IVector(IVector&& other) noexcept
{
    moveOrCopyFrom(std::move(other));
}

template<typename T, typename Storage>
template<typename OtherStorage>
IVector<T, Storage>::IVector(const IVector<T, OtherStorage>& other)
{
    copyFrom(other.cbegin(), other.size());
}

template<typename T, typename Storage>
template<typename OtherStorage>
IVector<T, Storage>::IVector(IVector<T, OtherStorage>&& other)
{
    destructElems(begin_, size());
    moveOrCopyFrom(std::move(other));
}

template<typename T, typename Storage>
IVector<T, Storage>& IVector<T, Storage>::operator=(std::initializer_list<T> il)
{
    destructElems(begin_, size());
    copyFrom(il.begin(), il.size());
    return *this;
}

template<typename T, typename Storage>
IVector<T, Storage>& IVector<T, Storage>::operator=(const IVector& other) noexcept
{
    if (this != &other)
    {
        destructElems(begin_, size());
        copyFrom(other.cbegin(), other.size());
    }
    return *this;
}

template<typename T, typename Storage>
IVector<T, Storage>& IVector<T, Storage>::operator=(IVector&& other) noexcept
{
    if (this != &other)
    {
        destructElems(begin_, size());
        moveOrCopyFrom(std::move(other));
    }
    return *this;
}

template<typename T, typename Storage>
template<typename OtherStorage>
IVector<T, Storage>& IVector<T, Storage>::operator=(const IVector<T, OtherStorage>& other) noexcept
{
    destructElems(begin_, size());
    copyFrom(other.cbegin(), other.size());
    return *this;
}

template<typename T, typename Storage>
template<typename OtherStorage>
IVector<T, Storage>& IVector<T, Storage>::operator=(IVector<T, OtherStorage>&& other) noexcept
{
    destructElems(begin_, size());
    moveOrCopyFrom(std::move(other));
    return *this;
}

template<typename T, typename Storage>
T& IVector<T, Storage>::operator[](int index)
{
    MK_ASSERT((0 <= index) && ((usize)index < capacity()));
    return begin_[index];
}

template<typename T, typename Storage>
const T& IVector<T, Storage>::operator[](int index) const
{
    MK_ASSERT((0 <= index) && ((usize)index < capacity()));
    return begin_[index];
}

template<typename T, typename Storage>
typename IVector<T, Storage>::iterator IVector<T, Storage>::begin() noexcept
{
    return begin_;
}

template<typename T, typename Storage>
typename IVector<T, Storage>::iterator IVector<T, Storage>::end() noexcept
{
    return end_;
}

template<typename T, typename Storage>
typename IVector<T, Storage>::const_iterator IVector<T, Storage>::begin() const noexcept
{
    return begin_;
}

template<typename T, typename Storage>
typename IVector<T, Storage>::const_iterator IVector<T, Storage>::end() const noexcept
{
    return end_;
}

template<typename T, typename Storage>
typename IVector<T, Storage>::const_iterator IVector<T, Storage>::cbegin() const noexcept
{
    return begin_;
}

template<typename T, typename Storage>
typename IVector<T, Storage>::const_iterator IVector<T, Storage>::cend() const noexcept
{
    return end_;
}

template<typename T, typename Storage>
T& IVector<T, Storage>::push(const T& v)
{
    return emplace(v);
}

template<typename T, typename Storage>
T& IVector<T, Storage>::push(T&& v)
{
    return emplace(std::move(v));
}

template<typename T, typename Storage>
T IVector<T, Storage>::pop()
{
    MK_ASSERT(size() > 0);
    T result = std::move(*(--end_));
    destructElems(end_, 1);
    return result;
}

template<typename T, typename Storage>
template<typename... ArgsType>
T& IVector<T, Storage>::emplace(ArgsType&&... args)
{
    ensureCapcity(size() + 1);
    ::new (static_cast<void*>(end_)) T(std::forward<ArgsType>(args)...);
    return *end_++;
}

template<typename T, typename Storage>
void IVector<T, Storage>::reserve(usize count)
{
    ensureCapcity(count);
}

template<typename T, typename Storage>
void IVector<T, Storage>::resize(usize count)
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

template<typename T, typename Storage>
void IVector<T, Storage>::shrinkToFit()
{
    usize goodCapcity = storage_.getGoodCapacity(size());
    if (goodCapcity >= capacity())
    {
        return;
    }

    usize currSize = size();
    begin_ = storage_.resize(begin_, goodCapcity);
    end_ = begin_ + currSize;
    capacity_ = begin_ + goodCapcity;
    MK_ASSERT(capacity() == goodCapcity);
}

template<typename T, typename Storage>
void IVector<T, Storage>::clear()
{
    destructElems(begin_, size());
    end_ = begin_;
}

template<typename T, typename Storage>
T* IVector<T, Storage>::data()
{
    return begin_;
}

template<typename T, typename Storage>
const T* IVector<T, Storage>::cdata() const
{
    return begin_;
}

template<typename T, typename Storage>
T& IVector<T, Storage>::back()
{
    MK_ASSERT(!empty());
    return *(end_ - 1);
}

template<typename T, typename Storage>
usize IVector<T, Storage>::size() const
{
    return end_ - begin_;
}

template<typename T, typename Storage>
bool IVector<T, Storage>::empty() const
{
    return !size();
}

template<typename T, typename Storage>
usize IVector<T, Storage>::capacity() const
{
    return capacity_ - begin_;
}

template<typename T, typename Storage>
void IVector<T, Storage>::ensureCapcity(usize requestedCapacity)
{
    usize currCapacity = capacity();
    if (requestedCapacity <= currCapacity)
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

template<typename T, typename Storage>
template<typename OtherStorage>
void IVector<T, Storage>::moveOrCopyFrom(IVector<T, OtherStorage>&& other)
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

template<typename T, typename Storage>
void IVector<T, Storage>::copyFrom(const T* src, usize count)
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

template<typename T, typename Storage>
void IVector<T, Storage>::destructElems(T* begin, usize count)
{
    if constexpr (!IsTriviallyDestructible<T>::value)
    {
        while (count--)
        {
            (begin++)->~T();
        }
    }
}

template<typename T, typename Storage>
void IVector<T, Storage>::constructElems(const T* otherData, usize count)
{
    T* it = begin();
    while (count--)
    {
        ::new (static_cast<void*>(it++)) T(*otherData++);
    }
}

template<typename T, typename Storage>
void IVector<T, Storage>::defaultConstructElems(T* begin, usize count)
{
    if constexpr (IsZeroInitializable<T>::value)
    {
        std::memset(begin, 0, sizeof(T) * count);
        return;
    }

    while (count--)
    {
        ::new (static_cast<void*>(begin++)) T;
    }
}

// -------------------------------- VectorView --------------------------------

template<typename T>
const T& VectorView<T>::operator[](int i) const
{
    MK_ASSERT(i < end_ - begin_);
    return begin_[i];
}

template<typename T>
T& VectorView<T>::operator[](int i)
{
    MK_ASSERT(i < end_ - begin_);
    return begin_[i];
}

template<typename T>
const T* VectorView<T>::cdata() const
{
    return begin_;
}

template<typename T>
T* VectorView<T>::data()
{
    return begin_;
}

template<typename T>
const T* VectorView<T>::cbegin() const
{
    return begin_;
}

template<typename T>
const T* VectorView<T>::cend() const
{
    return end_;
}

template<typename T>
T* VectorView<T>::begin()
{
    return begin_;
}

template<typename T>
T* VectorView<T>::end()
{
    return end_;
}

template<typename T>
usize VectorView<T>::size() const
{
    return end_ - begin_;
}

template<typename T>
bool VectorView<T>::empty() const
{
    return !size();
}

// ----------------------------------- util -----------------------------------
template<typename T>
VectorView<const byte> asBytes(VectorView<T> view)
{
    return VectorView<const byte>(reinterpret_cast<const byte*>(view.begin()),
                                  sizeof(T) * view.size());
}

template<typename T>
VectorView<byte> asWritableBytes(VectorView<T> view)
{
    return VectorView<byte>(reinterpret_cast<byte*>(view.begin()), sizeof(T) * view.size());
}

} // namespace mk
