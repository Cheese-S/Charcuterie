#pragma once

#include <iterator>
#ifndef MK_STRING_IMPL
    #include <core/container/IString.h>
#endif

namespace mk::details
{

template<typename Storage>
IString<Storage>::IString()
{
    data_.push('\0');
}

template<typename Storage>
IString<Storage>::IString(const char* cstr)
{
    MK_ASSERT(cstr);
    resize(strlen(cstr));
    memcpy(begin(), cstr, size());
}

template<typename Storage>
IString<Storage>::IString(const char* cstr, usize len)
{
    MK_ASSERT(cstr);
    resize(len);
    memcpy(begin(), cstr, size());
}

template<typename Storage>
IString<Storage>::IString(StringView view)
{
    resize(view.size());
    memcpy(data(), view.data(), view.size());
};

template<typename Storage>
template<typename OtherStorage>
bool IString<Storage>::operator==(const IString<OtherStorage>& other) const
{
    return size() == other.size() && memcmp(cbegin(), other.cbegin(), size() + 1) == 0;
}

template<typename Storage>
bool IString<Storage>::operator==(const IString& other) const
{
    return size() == other.size() && memcmp(cbegin(), other.cbegin(), size() + 1) == 0;
}

template<typename Storage>
bool IString<Storage>::operator==(const char* other) const
{
    return other && size() == strlen(other) && memcmp(cbegin(), other, size()) == 0;
}

template<typename Storage>
bool IString<Storage>::operator==(StringView other) const
{
    return size() == other.size() && memcmp(cbegin(), other.begin(), size()) == 0;
}

template<typename Storage>
IString<Storage> IString<Storage>::operator+(const IString& other)
{
    IString res = *this;
    res.push(other);
    return res;
}

template<typename Storage>
char& IString<Storage>::operator[](int index)
{
    MK_ASSERT((0 <= index) && ((usize)index < size()));
    return data_[index];
}

template<typename Storage>
char IString<Storage>::operator[](int index) const
{
    MK_ASSERT((0 <= index) && ((usize)index < size()));
    return data_[index];
}

template<typename Storage>
char* IString<Storage>::begin()
{
    return data_.begin();
}

template<typename Storage>
const char* IString<Storage>::cbegin() const
{
    return data_.cbegin();
}

template<typename Storage>
char* IString<Storage>::end()
{
    return data_.end() - 1;
}

template<typename Storage>
const char* IString<Storage>::cend() const
{
    return data_.cend() - 1;
}

template<typename Storage>
IString<Storage>::ReverseIt IString<Storage>::rbegin()
{
    return ReverseIt(end());
}

template<typename Storage>
IString<Storage>::ConstReverseIt IString<Storage>::crbegin() const
{
    return ConstReverseIt(cend());
}

template<typename Storage>
IString<Storage>::ReverseIt IString<Storage>::rend()
{
    return ReverseIt(begin());
}

template<typename Storage>
IString<Storage>::ConstReverseIt IString<Storage>::crend() const
{
    return ConstReverseIt(cbegin());
}

template<typename Storage>
void IString<Storage>::push(const IString& other)
{
    usize prevSize = size();
    resize(size() + other.size());
    memcpy(data() + prevSize, other.cbegin(), other.size());
}

template<typename Storage>
void IString<Storage>::push(const char* cstr)
{
    usize prevSize = size();
    usize otherLen = strlen(cstr);
    resize(size() + otherLen);
    memcpy(data() + prevSize, cstr, otherLen);
}

template<typename Storage>
void IString<Storage>::push(StringView view)
{
    usize prevSize = size();
    resize(size() + view.size());
    memcpy(data() + prevSize, view.begin(), view.size());
}

template<typename Storage>
void IString<Storage>::push(char c)
{
    data_[size()] = c;
    data_.push('\0');
}

template<typename Storage>
void IString<Storage>::replace(char from, char to)
{
    for (auto it = begin(); it != end(); it++)
    {
        if (*it == from)
        {
            *it = to;
        }
    }
}

template<typename Storage>
usize IString<Storage>::lfind(char c) const
{
    for (auto it = cbegin(); it != cend(); it++)
    {
        if (*it == c)
        {
            return it - cbegin();
        }
    }

    return kNpos;
}

template<typename Storage>
usize IString<Storage>::rfind(char c) const
{
    for (auto it = crbegin(); it != crend(); it++)
    {
        if (*it == c)
        {
            return it.base() - cbegin() - 1;
        }
    }
    return kNpos;
}

template<typename Storage>
template<typename... ArgsType>
void IString<Storage>::fmt(fmt::format_string<ArgsType...> format, ArgsType&&... args)
{
    FmtAdapter adapter(*this);
    fmt::format_to(std::back_inserter(adapter), format, std::forward<ArgsType>(args)...);
}

template<typename Storage>
const char* IString<Storage>::cstr() const
{
    return data_.cdata();
}

template<typename Storage>
char* IString<Storage>::data()
{
    return data_.data();
}

template<typename Storage>
usize IString<Storage>::size() const
{
    return data_.size() - 1;
}

template<typename Storage>
bool IString<Storage>::empty() const
{
    return size() == 0;
}

template<typename Storage>
void IString<Storage>::reserve(usize newCapacity)
{
    data_.reserve(newCapacity + 1);
}

template<typename Storage>
void IString<Storage>::resize(usize newSize)
{
    data_.resize(newSize + 1);
    data_[newSize] = '\0';
}

template<typename Storage>
void IString<Storage>::clear()
{
    resize(0);
}

template<typename Storage>
StringView IString<Storage>::cstrView()
{
    return StringView(begin(), data_.size());
}

} // namespace mk::details
