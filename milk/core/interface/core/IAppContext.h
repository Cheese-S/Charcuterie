#pragma once
#include <core/IMacro.h>
#include <core/IAssert.h>

namespace mk
{
template<typename T>
class AppContext
{
public:
    static T* tryGet()
    {
        return instance_;
    }

    static T& get()
    {
        MK_ASSERT(instance_);
        return *instance_;
    }

    static void registerIntsance(T* instance)
    {
        MK_ASSERT(instance_ == nullptr);
        instance_ = instance;
    }

    static void unregisterInstance()
    {
        MK_ASSERT(instance_);
        instance_ = nullptr;
    }

private:
    inline static T* instance_;
};
} // namespace mk
