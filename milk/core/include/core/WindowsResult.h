#pragma once

#include <core/IResult.h>
#include <core/os/IWindows.h>
#include <core/IAssert.h>

namespace mk
{
inline Result winErrorToResult(DWORD error)
{
    switch (error)
    {
    case ERROR_FILE_NOT_FOUND:
        return Result::eDoesNotExist;
    case ERROR_INSUFFICIENT_BUFFER:
        return Result::eOutOfCapactiy;
    case ERROR_ACCESS_DENIED:
        return Result::eInvalidPermission;
    case ERROR_ALREADY_EXISTS:
        return Result::eAlreadyExist;
    case ERROR_PATH_NOT_FOUND:
        return Result::eNotFound;
    case ERROR_SUCCESS:
        return Result::eOk;
    default:
        MK_ASSERT_UNREACHABLE();
        return Result::eUnknown;
    }
}
} // namespace mk
