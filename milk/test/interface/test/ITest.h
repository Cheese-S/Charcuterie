#pragma once

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <core/IAppContext.h>
#include <core/log/ISink.h>
#include <core/log/IFormatter.h>
#include <core/log/ILog.h>
#include <core/IUniquePtr.h>

#include <core/filesystem/IVfs.h>

namespace mk
{

#define EXPECT_OK(res)     EXPECT_TRUE(isOk((res)))
#define EXPECT_NOT_OK(res) EXPECT_TRUE(isNotOk((res)))
#define ASSERT_OK(res)     ASSERT_TRUE(isOk((res)))

class MilkEnvironment: public ::testing::Environment
{
public:
    explicit MilkEnvironment(StringView testName): testName_(testName) {}

    void SetUp() override
    {
        Result res = fs::Vfs::makeVfs(testName_, vfs_);
        EXPECT_OK(res);

        AppContext<fs::IVfs>::registerIntsance(vfs_.get());

        FixedVector<UniquePtr<log::ISink>, 2> sinks;
        sinks.push(makeUnique<log::ConsoleSink>());
        sinks.push(makeUnique<log::DebugSink>());

        log::LogSystemConfig config = { .pattern = log::LogSystemConfig::kDefaultPattern,
                                        .sinks = sinks,
                                        .userFlagFormatters = {} };

        EXPECT_OK(log::LogSystem::makeLogSystem(config, logSystem_));

        AppContext<log::LogSystem>::registerIntsance(logSystem_.get());
    }

    void TearDown() override
    {
        AppContext<log::LogSystem>::unregisterInstance();
        AppContext<fs::IVfs>::unregisterInstance();
    }

private:
    StringView                testName_;
    UniquePtr<log::LogSystem> logSystem_;
    UniquePtr<fs::IVfs>       vfs_;
};

} // namespace mk

#define MK_FULL_MAIN()                                                                        \
    int main(int argc, char** argv)                                                           \
    {                                                                                         \
        ::testing::AddGlobalTestEnvironment(new ::mk::MilkEnvironment("test_out/" __FILE__)); \
        ::testing::InitGoogleMock(&argc, argv);                                               \
        return RUN_ALL_TESTS();                                                               \
    } // namespace mk
