#include <gtest/gtest.h>
#include <atomic>
#include <chrono>
#include <thread>
#include <vector>
#include <core/concurrency/IParkingLot.h>

namespace mk::cc
{

using Clock = std::chrono::steady_clock;
using TimePoint = ParkingLot::TimePoint;

namespace
{

TimePoint forever()
{
    return TimePoint::max();
}
TimePoint expired()
{
    return Clock::now() - std::chrono::milliseconds(1);
}
TimePoint deadline(int ms)
{
    return Clock::now() + std::chrono::milliseconds(ms);
}
} // namespace

// ═════════════════════════════════════════════
// parkUntil — validation skip
// ═════════════════════════════════════════════

TEST(ParkingLot, ValidationFalseReturnsSkip)
{
    int  addr = 0;
    bool slept = false;

    ParkResult result = ParkingLot::parkUntil(
        &addr,
        0xFFFFFFFFU,
        []() { return false; },
        [&slept]() { slept = true; },
        forever());

    EXPECT_EQ(result, ParkResult::eSkip);
    EXPECT_FALSE(slept);
}

TEST(ParkingLot, BeforeSleepCalledWhenParking)
{
    int               addr = 0;
    std::atomic<bool> beforeSlept{ false };

    std::thread parker(
        [&]()
        {
            ParkResult result = ParkingLot::parkUntil(
                &addr,
                0xFFFFFFFFU,
                []() { return true; },
                [&beforeSlept]() { beforeSlept.store(true, std::memory_order_release); },
                forever());
            EXPECT_EQ(result, ParkResult::eUnparked);
        });

    while (!beforeSlept.load(std::memory_order_acquire))
    {
    }

    ParkingLot::unparkOne(&addr,
                          0xFFFFFFFFU,
                          [](UnparkResult result)
                          { EXPECT_EQ(result, UnparkResult::eNoneLeft); });

    parker.join();
    EXPECT_TRUE(beforeSlept.load());
}

// ═════════════════════════════════════════════
// parkUntil — timeout
// ═════════════════════════════════════════════

TEST(ParkingLot, ParkUntilExpiredDeadlineReturnsTimeout)
{
    int addr = 0;

    ParkResult result = ParkingLot::parkUntil(
        &addr,
        0xFFFFFFFFU,
        []() { return true; },
        []() {},
        expired());

    EXPECT_EQ(result, ParkResult::eTimeout);
}

TEST(ParkingLot, ParkUntilShortDeadlineReturnsTimeout)
{
    int addr = 0;

    ParkResult result = ParkingLot::parkUntil(
        &addr,
        0xFFFFFFFFU,
        []() { return true; },
        []() {},
        deadline(30));

    EXPECT_EQ(result, ParkResult::eTimeout);
}

// ═════════════════════════════════════════════
// unparkOne
// ═════════════════════════════════════════════

TEST(ParkingLot, UnparkOneWakesParkedThread)
{
    int               addr = 0;
    std::atomic<bool> sleeping{ false };
    ParkResult        parkResult = ParkResult::eSkip;

    std::thread parker(
        [&]()
        {
            parkResult = ParkingLot::parkUntil(
                &addr,
                0xFFFFFFFFU,
                []() { return true; },
                [&sleeping]() { sleeping.store(true, std::memory_order_release); },
                forever());
        });

    while (!sleeping.load(std::memory_order_acquire))
    {
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(5));

    ParkingLot::unparkOne(&addr,
                          0xFFFFFFFFU,
                          [](UnparkResult result)
                          { EXPECT_EQ(result, UnparkResult::eNoneLeft); });

    parker.join();
    EXPECT_EQ(parkResult, ParkResult::eUnparked);
}

TEST(ParkingLot, UnparkOneCallbackReceivesNoneLeftWhenSingleWaiter)
{
    int               addr = 0;
    std::atomic<bool> sleeping{ false };
    UnparkResult      captured = UnparkResult::eNotFound;

    std::thread parker(
        [&]()
        {
            ParkResult result = ParkingLot::parkUntil(
                &addr,
                0xFFFFFFFFU,
                []() { return true; },
                [&sleeping]() { sleeping.store(true, std::memory_order_release); },
                forever());
            EXPECT_EQ(result, ParkResult::eUnparked);
        });

    while (!sleeping.load(std::memory_order_acquire))
    {
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(5));

    ParkingLot::unparkOne(&addr,
                          0xFFFFFFFFU,
                          [&captured](UnparkResult result) { captured = result; });

    parker.join();
    EXPECT_EQ(captured, UnparkResult::eNoneLeft);
}

TEST(ParkingLot, UnparkOneCallbackReceivesMayHaveMoreWhenMultipleWaiters)
{
    int              addr = 0;
    std::atomic<int> sleeping{ 0 };
    constexpr int    kThreads = 3;
    UnparkResult     captured = UnparkResult::eNotFound;

    std::vector<std::thread> parkers;
    parkers.reserve(kThreads);

    for (int i = 0; i < kThreads; ++i)
    {
        parkers.emplace_back(
            [&]()
            {
                ParkResult result = ParkingLot::parkUntil(
                    &addr,
                    0xFFFFFFFFU,
                    []() { return true; },
                    [&sleeping]() { sleeping.fetch_add(1, std::memory_order_release); },
                    forever());
                EXPECT_EQ(result, ParkResult::eUnparked);
            });
    }

    while (sleeping.load(std::memory_order_acquire) < kThreads)
    {
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    ParkingLot::unparkOne(&addr,
                          0xFFFFFFFFU,
                          [&captured](UnparkResult result) { captured = result; });

    EXPECT_EQ(captured, UnparkResult::eMayHaveMore);

    ParkingLot::unparkAll(&addr, 0xFFFFFFFFU);
    for (auto& t : parkers)
    {
        t.join();
    }
}

TEST(ParkingLot, UnparkOneOnEmptyQueueGivesNotFound)
{
    int          addr = 0;
    UnparkResult captured = UnparkResult::eMayHaveMore;

    ParkingLot::unparkOne(&addr,
                          0xFFFFFFFFU,
                          [&captured](UnparkResult result) { captured = result; });

    EXPECT_EQ(captured, UnparkResult::eNotFound);
}

// ═════════════════════════════════════════════
// unparkAll
// ═════════════════════════════════════════════

TEST(ParkingLot, UnparkAllWakesAllParkedThreads)
{
    int              addr = 0;
    constexpr int    kThreads = 8;
    std::atomic<int> sleeping{ 0 };
    std::atomic<int> woken{ 0 };

    std::vector<std::thread> parkers;
    parkers.reserve(kThreads);

    for (int i = 0; i < kThreads; ++i)
    {
        parkers.emplace_back(
            [&]()
            {
                ParkResult result = ParkingLot::parkUntil(
                    &addr,
                    0xFFFFFFFFU,
                    []() { return true; },
                    [&sleeping]() { sleeping.fetch_add(1, std::memory_order_release); },
                    forever());
                EXPECT_EQ(result, ParkResult::eUnparked);
                woken.fetch_add(1, std::memory_order_relaxed);
            });
    }

    while (sleeping.load(std::memory_order_acquire) < kThreads)
    {
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    ParkingLot::unparkAll(&addr, 0xFFFFFFFFU);

    for (auto& t : parkers)
    {
        t.join();
    }

    EXPECT_EQ(woken.load(), kThreads);
}

TEST(ParkingLot, UnparkAllOnEmptyQueueIsNoop)
{
    int addr = 0;
    ParkingLot::unparkAll(&addr, 0xFFFFFFFFU);
}

// ═════════════════════════════════════════════
// Mask filtering
// ═════════════════════════════════════════════

TEST(ParkingLot, UnparkDoesNotWakeThreadWithNonMatchingMask)
{
    int               addr = 0;
    constexpr u32     kMaskA = 0x01U;
    constexpr u32     kMaskB = 0x02U;
    std::atomic<bool> sleeping{ false };
    ParkResult        parkResult = ParkResult::eSkip;

    std::thread parker(
        [&]()
        {
            parkResult = ParkingLot::parkUntil(
                &addr,
                kMaskA,
                []() { return true; },
                [&sleeping]() { sleeping.store(true, std::memory_order_release); },
                deadline(200));
        });

    while (!sleeping.load(std::memory_order_acquire))
    {
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(5));

    ParkingLot::unparkOne(&addr,
                          kMaskB,
                          [](UnparkResult result)
                          { EXPECT_EQ(result, UnparkResult::eNotFound); });

    parker.join();
    EXPECT_EQ(parkResult, ParkResult::eTimeout);
}

TEST(ParkingLot, UnparkWakesThreadWithMatchingMask)
{
    int               addr = 0;
    constexpr u32     kMask = 0x0FU;
    std::atomic<bool> sleeping{ false };
    ParkResult        parkResult = ParkResult::eSkip;

    std::thread parker(
        [&]()
        {
            parkResult = ParkingLot::parkUntil(
                &addr,
                kMask,
                []() { return true; },
                [&sleeping]() { sleeping.store(true, std::memory_order_release); },
                forever());
        });

    while (!sleeping.load(std::memory_order_acquire))
    {
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(5));

    ParkingLot::unparkOne(&addr,
                          0xFFU,
                          [](UnparkResult result)
                          { EXPECT_EQ(result, UnparkResult::eNoneLeft); });

    parker.join();
    EXPECT_EQ(parkResult, ParkResult::eUnparked);
}

// ═════════════════════════════════════════════
// Address isolation
// ═════════════════════════════════════════════

TEST(ParkingLot, UnparkOnDifferentAddressDoesNotWake)
{
    int               addrA = 0;
    int               addrB = 0;
    std::atomic<bool> sleeping{ false };
    ParkResult        parkResult = ParkResult::eSkip;

    std::thread parker(
        [&]()
        {
            parkResult = ParkingLot::parkUntil(
                &addrA,
                0xFFFFFFFFU,
                []() { return true; },
                [&sleeping]() { sleeping.store(true, std::memory_order_release); },
                deadline(200));
        });

    while (!sleeping.load(std::memory_order_acquire))
    {
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(5));

    ParkingLot::unparkOne(&addrB,
                          0xFFFFFFFFU,
                          [](UnparkResult result)
                          { EXPECT_EQ(result, UnparkResult::eNotFound); });

    parker.join();
    EXPECT_EQ(parkResult, ParkResult::eTimeout);
}

} // namespace mk::cc
