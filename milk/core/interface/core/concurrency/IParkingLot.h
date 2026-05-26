#pragma once
#include <mutex>
#include <core/IAssert.h>
#include <core/concurrency/IWordLock.h>
#include <core/concurrency/IScopedLock.h>
#include <core/util/IClock.h>
#include <core/IHash.h>

namespace mk::cc::details
{

struct ParkNode
{
    void*     address;
    ParkNode* next;
    ParkNode* prev;
    u32       mask;
    bool      shouldPark = false;

    std::mutex              mutex;
    std::condition_variable cv;
};

class ParkBucket
{
public:
    static details::ParkBucket& getBucket(u64 key);

    void push(ParkNode& node);
    void pop(ParkNode& node);

    ParkNode*        head;
    ParkNode*        tail;
    WordLock         lock;
    std::atomic<u32> count;
};

}; // namespace mk::cc::details

namespace mk::cc
{

enum class [[nodiscard]] ParkResult
{
    eUnparked,
    eTimeout,
    eSkip,
};

enum class [[nodiscard]] UnparkResult
{
    eNoneLeft,
    eMayHaveMore,
    eNotFound,
};

enum class [[nodiscard]] UnparkControl
{
    eStop,
    eContinue,
};

class ParkingLot
{
public:
    template<typename ValidationFn, typename BeforeSleepFn>
    static ParkResult parkUntil(void*           address,
                                u32             mask,
                                ValidationFn    validation,
                                BeforeSleepFn   beforeSleep,
                                util::TimePoint time);

    template<typename UnparkResultFn>
    static void unparkOne(void* address, u32 mask, UnparkResultFn callback);

    static void unparkAll(void* address, u32 mask);

private:
    template<typename UnparkControlFn>
    static void genericUnpark(void* address, u32 mask, UnparkControlFn controlFn);
};

template<typename ValidationFn, typename BeforeSleepFn>
ParkResult ParkingLot::parkUntil(void*           address,
                                 u32             mask,
                                 ValidationFn    validation,
                                 BeforeSleepFn   beforeSleep,
                                 util::TimePoint time)
{
    const u64 key = hash::twang_mix64(static_cast<u64>(reinterpret_cast<uptr>(address)));
    details::ParkBucket& bucket = details::ParkBucket::getBucket(key);
    details::ParkNode    self = {
           .address = address,
           .next = nullptr,
           .prev = nullptr,
           .mask = mask,
           .shouldPark = true,
           .mutex = {},
           .cv = {},
    };

    bucket.count.fetch_add(1);

    {
        ScopedLock<WordLock> bucketLock(bucket.lock);
        if (!validation())
        {
            bucket.count.fetch_add(-1, std::memory_order_relaxed);
            return ParkResult::eSkip;
        }

        bucket.push(self);
    }

    beforeSleep();

    std::cv_status status = std::cv_status::no_timeout;
    {
        std::unique_lock<std::mutex> lock(self.mutex);
        while (self.shouldPark && status != std::cv_status::timeout)
        {
            if (time == util::TimePoint::max())
            {
                // MK_RAW_LOG_INFO("Parked {}", mask);
                self.cv.wait(lock);
            }
            else
            {
                status = self.cv.wait_until(lock, time);
            }
        }
    }

    if (status == std::cv_status::timeout)
    {
        {
            ScopedLock<WordLock> lock(bucket.lock);
            if (self.shouldPark)
            {
                bucket.pop(self);
                return ParkResult::eTimeout;
            }
        }
    }

    MK_ASSERT(!self.shouldPark);
    return ParkResult::eUnparked;
}

template<typename UnparkResultFn>
void ParkingLot::unparkOne(void* address, u32 mask, UnparkResultFn callback)
{
    genericUnpark(address,
                  mask,
                  [callback](UnparkResult result)
                  {
                      callback(result);
                      return UnparkControl::eStop;
                  });
}

template<typename UnparkControlFn>
void ParkingLot::genericUnpark(void* address, u32 mask, UnparkControlFn controlFn)
{
    // MK_RAW_LOG_INFO("generic unpark: {}", mask);
    const u64 key = hash::twang_mix64(static_cast<u64>(reinterpret_cast<uptr>(address)));
    details::ParkBucket& bucket = details::ParkBucket::getBucket(key);

    UnparkControl        control;
    UnparkResult         result = UnparkResult::eNotFound;
    ScopedLock<WordLock> bucketLock(bucket.lock);
    details::ParkNode*   next;
    for (details::ParkNode* it = bucket.head; it != nullptr;)
    {
        next = it->next;
        if (it->address == address && it->mask & mask)
        {
            result =
                bucket.count > 1 ? UnparkResult::eMayHaveMore : UnparkResult::eNoneLeft;
            control = controlFn(result);
            bucket.pop(*it);
            {
                std::lock_guard<std::mutex> nodeLock(it->mutex);
                it->shouldPark = false;
                // MK_RAW_LOG_INFO("Wake up one");
                it->cv.notify_one();
            }

            if (control == UnparkControl::eStop)
            {
                return;
            }
        }
        it = next;
    }
    MK_IGNORE_RET_VAL controlFn(result);
}

} // namespace mk::cc
