#include <core/concurrency/IParkingLot.h>

namespace mk::cc::details
{
ParkBucket& ParkBucket::getBucket(u64 key)
{
    static constexpr u32       kBucketCount = 4096;
    static details::ParkBucket table[kBucketCount];

    return table[key % kBucketCount];
}

void ParkBucket::push(ParkNode& node)
{
    if (!head)
    {
        head = tail = &node;
        return;
    }

    tail->next = &node;
    node.prev = tail;
    tail = &node;
}

void ParkBucket::pop(ParkNode& node)
{
    if (&node == head && &node == tail)
    {
        head = nullptr;
        tail = nullptr;
    }
    else if (&node == head && &node != tail)
    {
        head = node.next;
        head->prev = nullptr;
    }
    else if (&node != head && &node == tail)
    {
        tail = node.prev;
        tail->next = nullptr;
    }
    else
    {
        node.prev->next = node.next;
        node.next->prev = node.prev;
    }
    count.fetch_sub(1, std::memory_order_relaxed);
}
} // namespace mk::cc::details

namespace mk::cc
{
void ParkingLot::unparkAll(void* address, u32 mask)
{
    genericUnpark(address,
                  mask,
                  [&](UnparkResult result)
                  {
                      if (result == UnparkResult::eNoneLeft ||
                          result == UnparkResult::eNotFound)
                      {
                          // MK_RAW_LOG_INFO("Unpark all just unparked");
                          return UnparkControl::eStop;
                      }
                      // MK_RAW_LOG_INFO("unpark continuing");
                      return UnparkControl::eContinue;
                  });
}
} // namespace mk::cc
