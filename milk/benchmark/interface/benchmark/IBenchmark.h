#pragma once

#include <benchmark/benchmark.h>

#include <core/IAppContext.h>
#include <core/log/ISink.h>
#include <core/log/IFormatter.h>
#include <core/log/ILog.h>
#include <core/IUniquePtr.h>
#include <core/filesystem/IVfs.h>

namespace mk
{

class MilkBenchmarkEnvironment
{
public:
    explicit MilkBenchmarkEnvironment(StringView benchmarkName): benchmarkName_(benchmarkName) {}

    bool setUp()
    {
        Result res = fs::Vfs::makeVfs(benchmarkName_, vfs_);
        if (isNotOk(res))
        {
            return false;
        }

        AppContext<fs::IVfs>::registerIntsance(vfs_.get());

        FixedVector<UniquePtr<log::ISink>, 2> sinks;
        sinks.push(makeUnique<log::ConsoleSink>());
        sinks.push(makeUnique<log::DebugSink>());

        log::LogSystemConfig config = { .pattern = log::LogSystemConfig::kDefaultPattern,
                                        .sinks = sinks,
                                        .userFlagFormatters = {} };

        res = log::LogSystem::makeLogSystem(config, logSystem_);
        if (isNotOk(res))
        {
            AppContext<fs::IVfs>::unregisterInstance();
            return false;
        }

        AppContext<log::LogSystem>::registerIntsance(logSystem_.get());
        return true;
    }

    void tearDown()
    {
        AppContext<log::LogSystem>::unregisterInstance();
        AppContext<fs::IVfs>::unregisterInstance();
    }

private:
    StringView                benchmarkName_;
    UniquePtr<log::LogSystem> logSystem_;
    UniquePtr<fs::IVfs>       vfs_;
};

} // namespace mk

#define MK_FULL_BENCHMARK_MAIN()                                       \
    int main(int argc, char** argv)                                    \
    {                                                                  \
        ::mk::MilkBenchmarkEnvironment env("benchmark_out/" __FILE__); \
        if (!env.setUp())                                              \
        {                                                              \
            return 1;                                                  \
        }                                                              \
        ::benchmark::Initialize(&argc, argv);                          \
        if (::benchmark::ReportUnrecognizedArguments(argc, argv))      \
        {                                                              \
            env.tearDown();                                            \
            return 1;                                                  \
        }                                                              \
        ::benchmark::RunSpecifiedBenchmarks();                         \
        ::benchmark::Shutdown();                                       \
        env.tearDown();                                                \
        return 0;                                                      \
    }
