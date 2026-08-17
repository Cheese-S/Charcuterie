#include <core/IMemory.h>
#include <swiss/core/PathTracer.h>
#include <core/log/IRawLog.h>

namespace mk::swiss
{
namespace
{
bool run()
{
    UniquePtr<PathTracer> pathTracer;
    Result                res = PathTracer::makePathTracer(pathTracer);
    if (!isOk(res))
    {
        return false;
    }
    return isOk(pathTracer->run());
}
} // namespace

} // namespace mk::swiss

int main()
{
    mk::mm::details::forceMiMallocLinkOrder();
    mk::swiss::run();
    return 0;
}
