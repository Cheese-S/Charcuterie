#pragma once

#include <gtest/gtest.h>
#include <core/IAppContext.h>
#include <core/log/ISink.h>
#include <core/log/IFormatter.h>
#include <core/log/ILog.h>
#include <core/IUniquePtr.h>

namespace mk
{

// NOLINTNEXTLINE
UniquePtr<log::LogSystem> gLogManager_ = nullptr;

#define EXPECT_OK(res)     EXPECT_TRUE(isOk((res)))
#define EXPECT_NOT_OK(res) EXPECT_TRUE(isNotOk((res)))

class MilkTest: public ::testing::Test
{
protected:
    static void SetUpTestSuite()
    {
        FixedVector<UniquePtr<log::ISink>, 2> sinks;
        sinks.push(makeUnique<log::ConsoleSink>());
        sinks.push(makeUnique<log::DebugSink>());

        log::LogSystemConfig config = { .pattern = log::LogSystemConfig::kDefaultPattern,
                                        .sinks = sinks,
                                        .userFlagFormatters = {} };

        EXPECT_OK(log::LogSystem::makeLogSystem(config, gLogManager_));

        AppContext<log::LogSystem>::registerIntsance(gLogManager_.get());
    }

    static void TearDownTestSuite()
    {
        log::LogSystem* logSystem = gLogManager_.release();
        delete logSystem;
        AppContext<log::LogSystem>::unregisterInstance();
    }

private:
};
} // namespace mk
