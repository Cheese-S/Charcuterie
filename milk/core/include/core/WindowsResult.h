#pragma once

#include <core/IResult.h>
#include <core/IWindows.h>
#include <core/IAssert.h>

namespace mk
{
inline Result winErrorToResult(DWORD error)
{
    switch (error)
    {
    case ERROR_FILE_NOT_FOUND:
        return mk::Result::eDoesNotExist;
    default:
        MK_ASSERT_UNREACHABLE();
        return mk::Result::eUnknown;
    }
}
} // namespace mk
