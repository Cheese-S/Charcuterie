#pragma once
#include <bit>
#include <core/IType.h>
#include <core/IMacro.h>
#include <core/ITraits.h>
#include <core/IResult.h>
#include <core/IAssert.h>

namespace mk
{
// Single threaded object pool
// Concurrency must be done externally
// It's NOT safe to destruct the pool if T is not a trivially destructable type
// All nodes must be released back to the pool first
template<typename T, usize Size>
class FixedSizeObjectPool
{
    struct Node
    {
        Node* next;
    };

public:
    static_assert(sizeof(T) >= sizeof(void*), "T must be at least as big as a pointer");
    static_assert(alignof(T) >= alignof(void*), "T must be aligned as big as a pointer");
    static_assert(Size >= 1, "MaxPoolSize must be at least 1");

    MK_NON_MOVABLE_NON_COPYABLE(FixedSizeObjectPool);

    FixedSizeObjectPool(): head_(nullptr)
    {
        for (usize i = 0; i < Size; i++)
        {
            push(reinterpret_cast<Node*>(storage_.bytes + i * sizeof(T)));
        }
    }

    ~FixedSizeObjectPool() = default;

    template<typename... ArgsType>
    T* tryAcquire(ArgsType&&... args)
    {
        if (!head_)
        {
            return nullptr;
        }

        Node* popped = head_;
        head_ = head_->next;

        T* object = ::new (static_cast<void*>(popped)) T(std::forward<ArgsType>(args)...);

        return object;
    }

    void release(T* object)
    {
        MK_ASSERT(isOwned(object));

        std::destroy_at(object);
        push(std::launder(reinterpret_cast<Node*>(object)));
    }

    bool isOwned(T* ptr)
    {
        uptr start = std::bit_cast<uptr>(&storage_.bytes);
        uptr end = start + (sizeof(T) * Size);
        uptr object = std::bit_cast<uptr>(ptr);

        return (start <= object && object < end) && ((object - start) % alignof(T) == 0);
    }

private:
    void push(Node* node)
    {
        node->next = head_;
        head_ = node;
    }

    TypeCompatibleBytesArray<T, Size> storage_;
    Node*                             head_;
};

} // namespace mk
