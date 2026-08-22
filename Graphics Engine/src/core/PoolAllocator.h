#pragma once

#include <array>
#include <assert.h>
#include <new>
#include <vector>

namespace Engine
{
// PoolAllocator:
// Allocates a large chunk of memory, and supports unordered creation / destruction of objects from this
// block.
// Create with:
// - T: Type of object this pool is managing
// - ElementCount: Expected number of elements this pool will handle
// The pool will manage a single page of memory with size T * ElementCount. If needed, the pool will allocate additional pages
// if allocations are made beyond the element count.
enum class PoolAllocatorPolicy : uint8_t
{
    FixedSize = 0,
    Growable = 1,
};
template <typename T,
          size_t ElementCount = 2048,
          PoolAllocatorPolicy Policy = PoolAllocatorPolicy::Growable>
class PoolAllocator
{
  private:
    static_assert(ElementCount <= 0xFFFF);
    struct MemoryPage
    {
        uint8_t data[sizeof(T) * ElementCount];

        std::array<uint16_t, ElementCount> freeList{};
        size_t freeListEnd = 0;

        MemoryPage* next = nullptr;

        MemoryPage()
        {
            memset(data, 0, sizeof(data));
            freeListEnd = 0;
            for (uint16_t i = ElementCount; i > 0; i--)
                freeList[freeListEnd++] = i - 1;
            next = nullptr;
        }
    };

    MemoryPage* page = nullptr;

  public:
    PoolAllocator() { page = nullptr; }
    ~PoolAllocator()
    {
        if constexpr (Policy == PoolAllocatorPolicy::FixedSize)
        {
            if (page)
                delete page;
        }
        else
        {
            MemoryPage* cursor = page;
            MemoryPage* next = nullptr;
            while (cursor != nullptr)
            {
                next = cursor->next;
                delete cursor;
                cursor = next;
            }
        }
    }

    template <typename... Args> T* allocate(Args&&... args)
    {
        // Find the first free page
        MemoryPage* targetPage = nullptr;

        if constexpr (Policy == PoolAllocatorPolicy::FixedSize)
        {
            if (!page)
                page = new MemoryPage();
            targetPage = page;
        }
        else
        {
            MemoryPage** targetPagePtr = &page;
            while (*targetPagePtr != nullptr &&
                   (*targetPagePtr)->freeListEnd == 0)
                targetPagePtr = &((*targetPagePtr)->next);
            if (*targetPagePtr == nullptr)
                *targetPagePtr = new MemoryPage();
            targetPage = *targetPagePtr;
        }

        // Allocate element
        assert(targetPage && targetPage->freeListEnd > 0);
        const uint32_t free_index =
            targetPage->freeList[--targetPage->freeListEnd];
        void* addr = &targetPage->data[free_index * sizeof(T)];
        T* ptr = ::new (addr) T(std::forward<Args>(args)...);

        return ptr;
    }
    void free(T* ptr)
    {
        assert(ptr);
        // Call destructor on element
        ptr->~T();

        // Find page that element corresponds to
        MemoryPage* targetPage = page;

        if constexpr (Policy != PoolAllocatorPolicy::FixedSize)
        {
            uint8_t* addr = reinterpret_cast<uint8_t*>(ptr);
            while (targetPage &&
                   !(targetPage->data <= addr &&
                     addr < targetPage->data + sizeof(targetPage->data)))
            {
                targetPage = targetPage->next;
            }
        }

        assert(targetPage);
        const uint32_t index =
            (uintptr_t(ptr) - uintptr_t(targetPage->data)) / sizeof(T);
        targetPage->freeList[targetPage->freeListEnd++] = index;
    }

    uint32_t getPageCount()
    {
        if constexpr (Policy == PoolAllocatorPolicy::FixedSize)
        {
            return page ? 1 : 0;
        }
        else
        {
            uint32_t count = 0;
            const MemoryPage* cursor = page;
            while (cursor != nullptr)
            {
                count++;
                cursor = cursor->next;
            }
            return count;
        }
    }
    size_t getNumAllocations()
    {
        size_t count = 0;

        if constexpr (Policy == PoolAllocatorPolicy::FixedSize)
        {
            if (page)
                count += ElementCount - page->freeListEnd;
        }
        else
        {
            const MemoryPage* cursor = page;
            while (cursor != nullptr)
            {
                count += ElementCount - cursor->freeListEnd;
                cursor = cursor->next;
            }
        }

        return count;
    }
    uint32_t getIndex(T* ptr)
    {
        if constexpr (Policy == PoolAllocatorPolicy::FixedSize)
        {
            if (page)
                return (uintptr_t(ptr) - uintptr_t(page->data)) / sizeof(T);
            else
                return 0xFFFF;
        }
        else
        {
            // Unimplemented
            assert(false);
            return 0;
        }
    }

    void* getData() { return page->data; }
    size_t getSize() { return ElementCount; }
};

} // namespace Engine