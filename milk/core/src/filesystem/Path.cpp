#include <core/filesystem/IPath.h>

namespace mk::fs
{

Path::Path(const String& str): str_(str)
{
    normalize();
};

Path::Path(String&& str): str_(std::move(str))
{
    normalize();
};

Path::Path(const char* cstr): str_(cstr)
{
    normalize();
}

Path& Path::operator/(const Path& other)
{
    if (other.str_.empty())
    {
        return *this;
    }

    if (other.isAbsolute())
    {
        str_ = other.str_;
        return *this;
    }

    str_.push(kSep);
    str_.push(other.str_);

    return *this;
}

Path& Path::operator/(const char* other)
{
    usize otherLen = strlen(other);

    if (otherLen == 0)
    {
        return *this;
    }

    if (other[0] == kSep || other[0] == kWindowsSep)
    {
        str_ = other;
    }
    else
    {
        str_.push(kSep);
        str_.push(other);
    }
    normalize();
    return *this;
}

void Path::normalize()
{
    str_.replace(kWindowsSep, kSep);
}

bool Path::isAbsolute() const
{
    return !str_.empty() && str_[0] == kSep;
}

StringView Path::suffix() const
{
    usize pos = str_.rfind('.');
    if (pos != String::kNpos)
    {
        return StringView(str_.cbegin() + pos + 1, str_.cend());
    }

    return "";
}

} // namespace mk::fs
