#include <core/concurrency/IWordLock.h>
#include <mutex>
#include <core/IAssert.h>
#include <core/IAtomic.h>
#include <core/concurrency/IPause.h>

namespace mk::cc::details
{
struct WaitNode
{
    std::mutex              mutex;
    std::condition_variable condition;
    bool                    shouldPark;

    // Tail will point to itself
    WaitNode* next;
    // The head will hold a valid ptr to the tail
    WaitNode* tail;
};

} // namespace mk::cc::details

namespace mk::cc
{

void WordLock::lockSlow()
{
    static constexpr u8 kMaxSpinCount = 40;

    uptr current;
    u8   spinCount = 0;

    for (;;)
    {
        current = addr_.load();
        if (!(current & kLockedBit))
        {
            MK_ASSERT(!(current & kQueueLockedBit));

            if (compareExchangeWeak(addr_, current, current | kLockedBit))
            {
                // Acquired the lock. Fast path
                return;
            }
        }

        // Adaptive spinning
        if (!(current & ~kAddrMask) && spinCount < kMaxSpinCount)
        {
            spinCount++;
            pause();
            continue;
        }

        details::WaitNode self;
        current = addr_.load();

        // If is not locked, or
        // somebody is holding the queue lock, or
        // failed to acquire the queu lock
        if (!(current & kLockedBit) || (current & kQueueLockedBit) ||
            !compareExchangeWeak(addr_, current, current | kQueueLockedBit))
        {
            std::this_thread::yield();
            continue;
        }

        // Now, has the queue lock, append self to the queue
        self.shouldPark = true;
        self.next = nullptr;

        details::WaitNode* head =
            std::bit_cast<details::WaitNode*>(current & ~(kAddrMask));

        // Queue is empty
        if (!head)
        {
            self.tail = &self;
            current = std::bit_cast<uptr>(&self);
            current |= kLockedBit;
        }
        else
        {
            head->tail->next = &self;
            head->tail = &self;
        }

        // Unlock queue lock
        current &= ~kQueueLockedBit;
        addr_ = current;

        {
            std::unique_lock<std::mutex> lock(self.mutex);
            while (self.shouldPark)
            {
                self.condition.wait(lock);
            }
        }
        // Someone wake us, time to try again
    }
}

void WordLock::unlockSlow()
{
    uptr current;
    for (;;)
    {
        current = addr_.load();
        if (current == kLockedBit)
        {
            if (compareExchangeWeak(addr_, current, static_cast<uptr>(0)))
            {
                // Release the lock, fast path;
                return;
            }

            std::this_thread::yield();
            continue;
        }

        if (current & kQueueLockedBit)
        {
            std::this_thread::yield();
            continue;
        }

        if (compareExchangeWeak(addr_, current, current | kQueueLockedBit))
        {
            break;
        }
    }

    current = addr_.load();
    MK_ASSERT(current & kLockedBit);
    MK_ASSERT(current & kQueueLockedBit);

    // Acquired the queue lock, now start popping the head
    details::WaitNode* popped = std::bit_cast<details::WaitNode*>(current & ~kAddrMask);
    MK_ASSERT(popped);
    if (popped->next)
    {
        popped->next->tail = popped->tail;
    }

    // Unlocks both queue lock and word lock
    current = std::bit_cast<uptr>(popped->next);
    addr_ = current;

    {
        std::scoped_lock<std::mutex> lock(popped->mutex);
        popped->shouldPark = false;
        popped->condition.notify_one();
    }
}

}; // namespace mk::cc
