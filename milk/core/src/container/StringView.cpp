#include <core/container/IString.h>

namespace mk
{
bool StringView::operator==(const StringView& other) const
{
    return size() == other.size() && memcmp(begin_, other.begin_, size()) == 0;
}

char StringView::operator[](int index) const
{
    MK_ASSERT((0 <= index) && ((usize)index < size()));
    return begin_[index];
}

const char* StringView::data() const
{
    return begin_;
}

const char* StringView::begin() const
{
    return begin_;
}

const char* StringView::end() const
{
    return end_;
}

StringView::ReverseIt StringView::rbegin() const
{
    return ReverseIt(end_);
}

StringView::ReverseIt StringView::rend() const
{
    return ReverseIt(begin_);
}

usize StringView::lfind(char c) const
{
    for (const char* it = begin(); it != end(); it++)
    {
        if (*it == c)
        {
            return it - begin();
        }
    }

    return kNpos;
}

usize StringView::rfind(char c) const
{
    for (auto it = rbegin(); it != rend(); it++)
    {
        if (*it == c)
        {
            return it.base() - begin() - 1;
        }
    }
    return kNpos;
}

StringView StringView::subview(usize i) const
{
    MK_ASSERT(i >= 0 && i < size());
    return StringView(begin_ + i, size() - i);
}

usize StringView::size() const
{
    return end_ - begin_;
}

bool StringView::empty() const
{
    return begin_ == nullptr || size();
}

StringView StringView::subview(usize i, usize count) const
{
    MK_ASSERT(i >= 0 && i < size());
    MK_ASSERT(i + count <= size());
    return StringView(begin_ + i, count);
}

} // namespace mk
