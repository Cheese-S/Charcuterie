#pragma once

namespace mk::fs
{
enum class AccessMode : int
{
    eRead = 0, // "r", file must exist
    eWrite,    // "w", create a new file if does not exist
    eAppend,   // "a", create a new file if does not exist

    eReadAndWrite,     // "r+", file must exist
    eReadAndOverwrite, // "w+", create a new file if does not exist
    eReadAndAppend,    // "a+", create a new file if does not exist
};

} // namespace mk::fs
