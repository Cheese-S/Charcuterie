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

// A memory allocation looks like this:
// ┌──────────────┬──────────────┬─────────────────────────┐
// │    Padding   │    Header    │      User Allocation    │
// └──────────────┴──────────────┴─────────────────────────┘
// <---------------------- MiMallocSize ------------------->
//                                <--- Aligned User Size -->
namespace
{

MemStats stats = {};

inline usize getRawSize(usize alignedUserSize, usize alignment)
{
    return details::align(alignedUserSize + kAllocHeaderSize, alignment);
}

inline void* getRaw(void* user, usize alignedUserSize, usize alignment)
{
    usize rawSize = getRawSize(alignedUserSize, alignment);
    return static_cast<u8*>(user) - (rawSize - alignedUserSize);
}

inline AllocHeader* getHeaderFromMiMalloc(void* miMalloc, usize alignedUserSize, usize alignment)
{
    usize rawSize = getRawSize(alignedUserSize, alignment);
    usize paddingSize = rawSize - kAllocHeaderSize - alignedUserSize;
    return reinterpret_cast<AllocHeader*>(static_cast<u8*>(miMalloc) + paddingSize);
}

inline AllocHeader* getHeaderFromUser(void* user)
{
    return reinterpret_cast<AllocHeader*>(static_cast<u8*>(user) - kAllocHeaderSize);
}

inline void* getUser(void* miMalloc, usize alignedUserSize, usize alignment)
{
    usize rawSize = getRawSize(alignedUserSize, alignment);
    return static_cast<u8*>(miMalloc) + (rawSize - alignedUserSize);
}

inline void updateStats(i64 change)
{
    // std::cout << "update stats: " << change << std::endl;
    stats.size.fetch_add(change, std::memory_order_relaxed);
}

inline void updateHeader(void* headerAddr, usize size, usize alignment)
{
    AllocHeader* header = static_cast<AllocHeader*>(headerAddr);
    header->size = size;
    header->magic = kHeaderMagic;
    header->alignment = alignment;
}

} // namespace

void* alloc(usize userSize, usize alignment)
{
    usize alignedUserSize = details::align(userSize, alignment);
    void* raw = mi_malloc_aligned(getRawSize(alignedUserSize, alignment), alignment);
    void* header = getHeaderFromMiMalloc(raw, alignedUserSize, alignment);
    updateHeader(header, userSize, alignment);
    updateStats(userSize);
    void* user = getUser(raw, alignedUserSize, alignment);
    MK_ASSERT(std::bit_cast<uptr>(user) % alignment == 0);
    return user;
}

void* realloc(void* user, usize newSize, usize alignment)
{
    void* raw = nullptr;
    i64   change = newSize;
    if (user)
    {
        AllocHeader* header = getHeaderFromUser(user);
        usize        alignedUserSize = details::align(header->size, header->alignment);
        raw = getRaw(user, alignedUserSize, header->alignment);
        change -= header->size;
    }
    usize alignedNewSize = details::align(newSize, alignment);
    void* newRaw = mi_realloc_aligned(raw, getRawSize(alignedNewSize, alignment), alignment);
    void* header = getHeaderFromMiMalloc(newRaw, alignedNewSize, alignment);
    void* newUser = getUser(newRaw, alignedNewSize, alignment);
    updateStats(change);
    updateHeader(header, newSize, alignment);
    MK_ASSERT(std::bit_cast<uptr>(newUser) % alignment == 0);
    return newUser;
}

void free(void* user)
{
    if (!user)
    {
        return;
    }

    AllocHeader* header = getHeaderFromUser(user);

    MK_ASSERTF(header->magic != kScrambledMagic, "detected double free");
    MK_ASSERT(header->magic == kHeaderMagic);

    header->magic = kScrambledMagic;
    updateStats(-header->size);
    usize alignedUserSize = details::align(header->size, header->alignment);
    mi_free(getRaw(user, alignedUserSize, header->alignment));
}

usize getGoodSize(usize requestedSize)
{
    if (!requestedSize)
    {
        return 0;
    }

    return mi_good_size(requestedSize);
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

mi_decl_new_nothrow(n) void* operator new(std::size_t n, const std::nothrow_t& tag) noexcept
{
    MK_UNREF(tag);
    return mk::mm::alloc(n, mk::mm::details::kDefaultAlignment);
}
mi_decl_new_nothrow(n) void* operator new[](std::size_t n, const std::nothrow_t& tag) noexcept
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
