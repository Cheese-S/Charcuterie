#pragma once
#include <bit>
#include <algorithm>

#include <core/IType.h>
#include <core/ITraits.h>
#include <core/IAtomic.h>
#include <core/IResult.h>
#include <core/IAssert.h>
#include <core/concurrency/ISharedMutex.h>
#include <core/IMemory.h>
#include <core/concurrency/IScopedLock.h>

namespace mk
{

template<typename T, usize PerChunkObjectCount>
class LockFreeObjectPool
{
    static constexpr usize kNodeAlignment = std::max(alignof(T), 64LLU);

    struct Chunk;

    struct alignas(kNodeAlignment) Node
    {
        static constexpr u32   kRefCountMask = 0x7FFFFFFF;
        static constexpr u32   kShouldBeOnFreeListBit = 0x80000000;
        // [0 - 30] ref count, [31] shouldBeOnFreelist
        TypeCompatibleBytes<T> storage;
        std::atomic<u32>       ref;
        std::atomic<Node*>     next;
        Chunk* chunk; // This is set once in listify and never changed again
    };

    struct Chunk
    {
        TypeCompatibleBytesArray<Node, PerChunkObjectCount> storage;
        Chunk*                                              next;
        usize index; // This is set once and never changed again
    };

    static_assert(alignof(Node) % 64 == 0,
                  "Size of node must be 64 to avoid false sharing");

public:
    static_assert(PerChunkObjectCount >= 1, "MaxPoolSize must be at least 1");

    LockFreeObjectPool(usize maxChunkCount):
        maxChunkCount_(maxChunkCount), currentChunkCount_(1), chunks_(nullptr)
    {
        MK_ASSERTF(maxChunkCount >= 1, "Must allow at least one chunk.");
        Node* head = nullptr;
        Node* tail = nullptr;
        listify(base_, head, tail);
        head_.store(head, std::memory_order_release);
        base_.next = nullptr;
        base_.index = 0;
        chunks_ = &base_;
    }

    ~LockFreeObjectPool()
    {
        // The tail is the base. Don't free it
        MK_ASSERT(chunks_);
        while (chunks_->next)
        {
            Chunk* next = chunks_->next;
            mm::free(chunks_);
            chunks_ = next;
        }
    };

    void release(T* object)
    {
        MK_ASSERT(isOwned(object));

        std::destroy_at(object);
        Node* node = std::launder(reinterpret_cast<Node*>(object));
        if (node->ref.fetch_add(Node::kShouldBeOnFreeListBit,
                                std::memory_order_release) == 0)
        {
            pushNode(node);
        }
    }

    usize index(T* object)
    {
        MK_ASSERT(isOwned(object));
        Node* node = reinterpret_cast<Node*>(object);

        uptr start = std::bit_cast<uptr>(&node->chunk->storage.bytes);
        uptr nodeAddr = std::bit_cast<uptr>(node);

        usize chunkLocalIndex = (nodeAddr - start) / sizeof(Node);
        return chunkLocalIndex + PerChunkObjectCount * node->chunk->index;
    }

    template<typename... ArgsType>
    T* tryAcquire(ArgsType&&... args)
    {
        return tryAcquireImpl(true, std::forward<ArgsType>(args)...);
    }

private:
    void listify(Chunk& chunk, Node*& outHead, Node*& outTail)
    {
        Node* head = nullptr;
        for (usize i = 0; i < PerChunkObjectCount; i++)
        {
            Node* node = reinterpret_cast<Node*>(chunk.storage.bytes + sizeof(Node) * i);
            node->next.store(head, std::memory_order_relaxed);
            node->ref = 1;
            head = node;
            node->chunk = &chunk;
        }
        outHead = head;
        outTail = reinterpret_cast<Node*>(chunk.storage.bytes +
                                          sizeof(Node) * (PerChunkObjectCount - 1));
    }

    bool isOwned(T* ptr)
    {
        uptr object = std::bit_cast<uptr>(ptr);
        uptr end;
        uptr start;

        {
            cc::ScopedShareLock<cc::SharedMutex> lock(chunksMutex_);

            Chunk* chunk = chunks_;
            while (chunk)
            {
                start = std::bit_cast<uptr>(&chunk->storage.bytes);
                end = start + sizeof(Node) * PerChunkObjectCount;
                if ((start <= object && object < end) &&
                    ((object - start) % alignof(Node) == 0))
                {
                    return true;
                }
                chunk = chunk->next;
            }
        }

        return false;
    }

    void pushNode(Node* node)
    {
        // We should be the only thread who enters this function.
        // tryAcquire cannot proceed when the node's ref == 0
        Node* head = head_.load(std::memory_order_relaxed);
        while (true)
        {
            // Remove the flag + add list ref
            node->next.store(head, std::memory_order_relaxed);
            node->ref.store(1, std::memory_order_release);

            if (!head_.compare_exchange_strong(head,
                                               node,
                                               std::memory_order_release,
                                               std::memory_order_relaxed))
            {
                // We remove our ref + setting the flag again.
                if (node->ref.fetch_add(Node::kShouldBeOnFreeListBit - 1,
                                        std::memory_order_acq_rel) == 1)
                {
                    // Now, the ref is back to 0 again, i.e. we now have control of the
                    // node again
                    continue;
                }
            }
            // It's the other thread's responsibility to now push this node back on the
            // list
            return;
        }
    }

    template<typename... ArgsType>
    T* tryAcquireImpl(bool allowPushChunk, ArgsType&&... args)
    {
        Node* popped = head_.load(std::memory_order_acquire);
        Node* next;
        while (popped)
        {
            Node* prevHead = popped;
            u32   poppedRef = popped->ref.load(std::memory_order_relaxed);

            // Try to add a ref when ref count != 0
            if ((poppedRef & Node::kRefCountMask) == 0 ||
                !popped->ref.compare_exchange_strong(poppedRef,
                                                     poppedRef + 1,
                                                     std::memory_order_acquire))
            {
                popped = head_.load(std::memory_order_acquire);
                continue;
            }

            // Here, popped->next can no longer change since ref > 0. (we just added one)
            // Free to grab next and cas
            next = popped->next.load(std::memory_order_relaxed);
            if (head_.compare_exchange_strong(popped,
                                              next,
                                              std::memory_order_acquire,
                                              std::memory_order_relaxed))
            {
                // We got the node. Remove 2 refs (the ref we added + the list one: see
                // pushNode)
                MK_ASSERT(!(popped->ref.load(std::memory_order_relaxed) &
                            Node::kShouldBeOnFreeListBit));
                popped->ref.fetch_add(-2, std::memory_order_relaxed);

                T* object = ::new (static_cast<void*>(&popped->storage.bytes))
                    T(std::forward<ArgsType>(args)...);

                return object;
            }

            // Someone returned prevHead before we can.
            // We need to decrement the ref count
            if (prevHead->ref.fetch_add(-1, std::memory_order_acq_rel) ==
                Node::kShouldBeOnFreeListBit + 1)
            {
                // Push it back on the queue if prevHead is released back to the pool
                pushNode(prevHead);
            }
        }

        usize currentChunkCount = currentChunkCount_.load(std::memory_order_relaxed);
        if (allowPushChunk && currentChunkCount < maxChunkCount_)
        {
            {
                cc::ScopedLock<cc::SharedMutex> lock(chunksMutex_);

                // Possible that some other thread has already made a new chunk
                T* object = tryAcquireImpl(false, std::forward<ArgsType>(args)...);
                if (object)
                {
                    return object;
                }

                currentChunkCount_.fetch_add(1, std::memory_order_relaxed);

                Chunk* chunk = static_cast<Chunk*>(mm::alloc(sizeof(Chunk)));
                chunk->next = chunks_;
                chunk->index = currentChunkCount + 1;
                chunks_ = chunk;

                Node* tail = nullptr;
                listify(*chunk, popped, tail);
                Node* newHead = popped->next;
                Node* oldHead = head_.load(std::memory_order_acquire);

                do
                {
                    tail->next = oldHead;
                } while (!head_.compare_exchange_weak(oldHead,
                                                      newHead,
                                                      std::memory_order_acquire,
                                                      std::memory_order_relaxed));
            }

            MK_ASSERT(popped);

            return ::new (static_cast<void*>(&popped->storage.bytes))
                T(std::forward<ArgsType>(args)...);
        }

        return nullptr;
    }

    Chunk              base_;
    std::atomic<Node*> head_;
    const usize        maxChunkCount_;
    std::atomic<usize> currentChunkCount_;

    Chunk*          chunks_;
    cc::SharedMutex chunksMutex_;
};

} // namespace mk
