#pragma once
#include <initializer_list>

#include <core/IResult.h>
#include <core/ITraits.h>
#include <core/IType.h>
#include <core/IMemory.h>
#include <core/IAssert.h>

namespace mk
{

template<typename T, typename Storage>
class IVector
{
public:
    using iterator = T*;
    using const_iterator = const T*;

    IVector();
    ~IVector();

    IVector(std::initializer_list<T> il);
    IVector(const IVector& other);
    IVector(IVector&& other) noexcept;

    template<typename OtherStorage>
    IVector(const IVector<T, OtherStorage>& other);

    template<typename OtherStorage>
    IVector(IVector<T, OtherStorage>&& other);

    IVector& operator=(std::initializer_list<T> il);
    IVector& operator=(const IVector& other) noexcept;
    IVector& operator=(IVector&& other) noexcept;

    template<typename OtherStorage>
    IVector& operator=(const IVector<T, OtherStorage>& other) noexcept;

    template<typename OtherStorage>
    IVector& operator=(IVector<T, OtherStorage>&& other) noexcept;

    T&       operator[](int index);
    const T& operator[](int index) const;

    iterator begin() noexcept;
    iterator end() noexcept;

    const_iterator begin() const noexcept;
    const_iterator end() const noexcept;

    const_iterator cbegin() const noexcept;
    const_iterator cend() const noexcept;

    T& push(const T& v);
    T& push(T&& v);

    T pop();

    template<typename... ArgsType>
    T& emplace(ArgsType&&... args);

    void reserve(usize count);
    void resize(usize count);
    void clear();

    T*       data();
    const T* cdata() const;

    [[nodiscard]] usize size() const;
    [[nodiscard]] bool  empty() const;
    [[nodiscard]] usize capacity() const;

private:
    void ensureCapcity(usize requestedCapacity);

    template<typename OtherStorage>
    void moveOrCopyFrom(IVector<T, OtherStorage>&& other);

    void copyFrom(const T* src, usize count);
    void destructElems(T* begin, usize count);
    void constructElems(const T* otherData, usize count);
    void defaultConstructElems(T* begin, usize count);

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

    const T& operator[](int i) const;
    T&       operator[](int i);

    const T* cdata() const;
    T*       data();
    const T* cbegin() const;
    const T* cend() const;
    T*       begin();
    T*       end();

    usize              size() const;
    [[nodiscard]] bool empty() const;

private:
    T* begin_;
    T* end_;
};

template<typename T>
VectorView<const byte> asBytes(VectorView<T> view);

template<typename T>
VectorView<byte> asWritableBytes(VectorView<T> view);

} // namespace mk
#define MK_VECTOR_IMPL
#include <core/container/IVector.inl>
#undef MK_VECTOR_IMPL
