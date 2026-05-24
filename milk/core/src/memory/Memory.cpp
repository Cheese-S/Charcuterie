#include <core/IMacro.h>
#include <core/IMemory.h>
#include <core/Memory.h>
#include <core/IAssert.h>
#include <new>
#include <iostream>

namespace mk::mm::details
{
void forceMiMallocLinkOrder()
{
    mi_version();
}
} // namespace mk::mm::details

namespace mk::mm
{

// TODO(Cheese_S): make stats thread safe
// TODO(Cheese_S): implement leak detector

// ---------------------------------------- HEAP ALLOCATOR
// ----------------------------------------

namespace
{

details::MemStats stats = {};

inline details::AllocHeader* getHeader(void* userPtr)
{
    return reinterpret_cast<details::AllocHeader*>(static_cast<u8*>(userPtr) -
                                                   details::kAllocHeaderSize);
}

inline void* getUserPtrAddr(void* headerPtr)
{
    return static_cast<u8*>(headerPtr) + details::kAllocHeaderSize;
}

inline void updateStats(i64 change)
{
    // std::cout << "update stats: " << change << std::endl;
    stats.size.fetch_add(change, std::memory_order_relaxed);
}

inline void updateHeader(void* headerAddr, usize size)
{
    details::AllocHeader* header = static_cast<details::AllocHeader*>(headerAddr);
    header->size = size;
    header->magic = details::kHeaderMagic;
}

} // namespace

void* alloc(usize requestSize, usize alignment)
{
    void* ptr = mi_malloc_aligned(details::kAllocHeaderSize + requestSize, alignment);
    updateHeader(ptr, requestSize);
    updateStats(requestSize);
    return getUserPtrAddr(ptr);
}

void* realloc(void* oldPtr, usize newSize, usize alignment)
{
    i64 change = newSize;
    if (oldPtr != nullptr)
    {
        details::AllocHeader* header = getHeader(oldPtr);
        change -= header->size;
        oldPtr = header;
    }
    void* newPtr =
        mi_realloc_aligned(oldPtr, newSize + details::kAllocHeaderSize, alignment);
    updateStats(change);
    updateHeader(newPtr, newSize);
    return getUserPtrAddr(newPtr);
}

void free(void* ptr)
{
    if (ptr == nullptr)
    {
        return;
    }
    details::AllocHeader* header = getHeader(ptr);

    if (header->magic == details::kScrambledMagic)
    {
        MK_ASSERTF(false, "detected double free");
    }
    MK_ASSERT(header->magic == details::kHeaderMagic);
    header->magic = details::kScrambledMagic;
    updateStats(-header->size);
    mi_free(header);
}

i64 getMemSize()
{
    return stats.size.load(std::memory_order_relaxed);
};

} // namespace mk::mm

#if defined(_MSC_VER) && defined(_Ret_notnull_) && defined(_Post_writable_byte_size_)
  // stay consistent with VCRT definitions
    #define mi_decl_new(n) \
        mi_decl_nodiscard mi_decl_restrict _Ret_notnull_ _Post_writable_byte_size_(n)
    #define mi_decl_new_nothrow(n)                                                   \
        mi_decl_nodiscard mi_decl_restrict _Ret_maybenull_ _Success_(return != NULL) \
            _Post_writable_byte_size_(n)
#else
    #define mi_decl_new(n)         mi_decl_nodiscard mi_decl_restrict
    #define mi_decl_new_nothrow(n) mi_decl_nodiscard mi_decl_restrict
#endif

void operator delete(void* p) noexcept
{
    mk::mm::free(p);
};
void operator delete[](void* p) noexcept
{
    mk::mm::free(p);
};

void operator delete(void* p, const std::nothrow_t&) noexcept
{
    mk::mm::free(p);
}
void operator delete[](void* p, const std::nothrow_t&) noexcept
{
    mk::mm::free(p);
}

mi_decl_new(n) void* operator new(std::size_t n) noexcept(false)
{
    return mk::mm::alloc(n, mk::mm::details::kDefaultAlignment);
}
mi_decl_new(n) void* operator new[](std::size_t n) noexcept(false)
{
    return mk::mm::alloc(n, mk::mm::details::kDefaultAlignment);
}

mi_decl_new_nothrow(n) void* operator new(std::size_t           n,
                                          const std::nothrow_t& tag) noexcept
{
    MK_UNREF(tag);
    return mk::mm::alloc(n, mk::mm::details::kDefaultAlignment);
}
mi_decl_new_nothrow(n) void* operator new[](std::size_t           n,
                                            const std::nothrow_t& tag) noexcept
{
    MK_UNREF(tag);
    return mk::mm::alloc(n, mk::mm::details::kDefaultAlignment);
}

#if (__cplusplus >= 201402L || _MSC_VER >= 1916)
void operator delete(void* p, std::size_t n) noexcept
{
    MK_UNREF(n);
    mk::mm::free(p);
};
void operator delete[](void* p, std::size_t n) noexcept
{
    MK_UNREF(n);
    mk::mm::free(p);
};
#endif

#if (__cplusplus > 201402L || defined(__cpp_aligned_new))
void operator delete(void* p, std::align_val_t al) noexcept
{
    MK_UNREF(al);
    mk::mm::free(p);
}
void operator delete[](void* p, std::align_val_t al) noexcept
{
    MK_UNREF(al);
    mk::mm::free(p);
}
void operator delete(void* p, std::size_t n, std::align_val_t al) noexcept
{
    MK_UNREF(n);
    MK_UNREF(al);
    mk::mm::free(p);
};
void operator delete[](void* p, std::size_t n, std::align_val_t al) noexcept
{
    MK_UNREF(n);
    MK_UNREF(al);
    mk::mm::free(p);
};
void operator delete(void* p, std::align_val_t al, const std::nothrow_t&) noexcept
{
    MK_UNREF(al);
    mk::mm::free(p);
}
void operator delete[](void* p, std::align_val_t al, const std::nothrow_t&) noexcept
{
    MK_UNREF(al);
    mk::mm::free(p);
}

void* operator new(std::size_t n, std::align_val_t al) noexcept(false)
{
    return mk::mm::alloc(n, static_cast<size_t>(al));
}
void* operator new[](std::size_t n, std::align_val_t al) noexcept(false)
{
    return mk::mm::alloc(n, static_cast<size_t>(al));
}
void* operator new(std::size_t n, std::align_val_t al, const std::nothrow_t&) noexcept
{
    return mk::mm::alloc(n, static_cast<size_t>(al));
}
void* operator new[](std::size_t n, std::align_val_t al, const std::nothrow_t&) noexcept
{
    return mk::mm::alloc(n, static_cast<size_t>(al));
}
#endif
