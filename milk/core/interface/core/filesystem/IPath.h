#pragma once
#include <core/container/IString.h>

namespace mk::fs
{
class Path
{
    static const char kSep = '/';
    static const char kWindowsSep = '\\';

public:
    MK_DEFAULT_MOVABLE_DEFAULT_COPYABLE(Path);

    explicit Path(const String& str);
    explicit Path(String&& str);
    explicit Path(const char* cstr);

    Path& operator/(const Path& other);
    Path& operator/(const char* other);

    // Return true if path is the form "/{}"
    // On winows, this technically is path from the root of the current drive
    // Current drive: C:\\, then "/abc" = "C:\\abc"
    bool isAbsolute() const;

    // aaa.abc.txt -> "txt"
    // aaa -> ""
    StringView suffix() const;

    const char* cstr() const;

private:
    void   normalize();
    String str_;
};

inline const char* Path::cstr() const
{
    return str_.cstr();
}

} // namespace mk::fs
