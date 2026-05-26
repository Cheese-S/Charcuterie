#pragma once

#include <core/IType.h>

namespace mk::cc
{

class IJob
{
public:
    virtual void run() = 0;
    virtual ~IJob() = default;

private:
};

struct SmallJobStorage
{
    alignas(64) byte bytes[64];
};

struct MediumJobStorage
{
    alignas(64) byte bytes[128];
};

struct LargeJobStorage
{
    alignas(64) byte bytes[256];
};

}; // namespace mk::cc
