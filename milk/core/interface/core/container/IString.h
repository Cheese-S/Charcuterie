#pragma once

#include <core/IMemory.h>
#include <cstring>
#include <core/IResult.h>
#include <core/IType.h>
#include <core/container/IVector.h>
#include <core/IMacro.h>
#include <core/IAssert.h>

namespace mk
{
class StringView;
}

namespace mk::details
{
constexpr usize strlen(const char* cstr)
{
    usize len = 0;
    while (*cstr != '\0')
    {
        len++;
        cstr++;
    }
    return len;
}

template<typename Storage>
class IString
{
    using ReverseIt = std::reverse_iterator<char*>;
    using ConstReverseIt = std::reverse_iterator<const char*>;

public:
    static constexpr usize kNpos = SIZE_MAX;

    MK_DEFAULT_MOVABLE_DEFAULT_COPYABLE(IString);
    IString();
    IString(const char* cstr);
    IString(const char* cstr, usize len);
    explicit IString(StringView view);

    template<typename OtherStorage>
    bool operator==(const IString<OtherStorage>& other) const;
    bool operator==(const IString& other) const;
    bool operator==(StringView other) const;
    bool operator==(const char* other) const;

    IString operator+(const IString& other);

    char& operator[](int index);
    char  operator[](int index) const;

    char*       begin();
    const char* cbegin() const;
    char*       end();
    const char* cend() const;

    ReverseIt      rbegin();
    ConstReverseIt crbegin() const;
    ReverseIt      rend();
    ConstReverseIt crend() const;

    void push(const IString& other);
    void push(StringView view);
    void push(const char* cstr);
    void push(char c);

    void  replace(char from, char to);
    usize lfind(char c) const;
    usize rfind(char c) const;

    const char* cstr() const;
    StringView  cstrView();
    char*       data();

    [[nodiscard]] usize size() const;
    [[nodiscard]] bool  empty() const;

    void reserve(usize newCapacity);
    void resize(usize newSize);
    void clear();

private:
    IVector<char, Storage> data_;
};

} // namespace mk::details

namespace mk
{
using String = details::IString<mm::LinearHeapStorage<char>>;

template<usize N>
using StackString = details::IString<mm::LinearStackStorage<char, N + 1>>;

// TODO(Cheese_S): Rethink how empty view should work. nullptr or pointing to a '\0'
class StringView
{
public:
    static constexpr usize kNpos = SIZE_MAX;

    using ReverseIt = std::reverse_iterator<const char*>;

    explicit StringView(const char* begin, const char* end): begin_(begin), end_(end) {};
    explicit constexpr StringView(const char* begin, size_t count):
        begin_(begin), end_(begin_ + count) {};
    constexpr StringView(): begin_(nullptr), end_(nullptr) {};
    constexpr StringView(const char* begin): // NOLINT
        begin_(begin), end_(begin + details::strlen(begin))
    {
    }
    explicit StringView(const String& str): begin_(str.cbegin()), end_(str.cend()) {}

    bool operator==(const StringView& other) const;

    char operator[](int index) const;

    [[nodiscard]] const char* data() const;
    [[nodiscard]] const char* begin() const;
    [[nodiscard]] const char* end() const;
    [[nodiscard]] ReverseIt   rbegin() const;
    [[nodiscard]] ReverseIt   rend() const;

    usize lfind(char c) const;
    usize rfind(char c) const;

    [[nodiscard]] usize size() const;
    [[nodiscard]] bool  empty() const;

    StringView subview(usize i, usize count) const;
    StringView subview(usize i) const;

private:
    const char* begin_;
    const char* end_;
};

} // namespace mk
  //
#define MK_STRING_IMPL
#include <core/container/IString.inl>
#undef MK_STRING_IMPL
