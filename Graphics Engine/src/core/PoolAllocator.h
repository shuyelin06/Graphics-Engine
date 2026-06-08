#pragma once

#include <assert.h>
#include <new>
#include <vector>

namespace Engine {
// ArenaAllocator:
// Simple implementation of an arena allocator.
// This allocates a large chunk of memory, which is used for the arena's entire
// lifetime.
// On "allocation" and "free", this allocator returns pointers to regions of
// this block of memory.
template <typename T, size_t SIZE> class PoolAllocator {
    std::vector<uint8_t> data;
    std::vector<uint16_t> free_indices;

  public:
    PoolAllocator() {
        data.resize(sizeof(T) * SIZE);
        // Add to free indices in descending order, so that when we pop we use
        // free indices from 0 --> size (better for caching)s
        for (uint16_t i = SIZE; i > 0; i--) {
            free_indices.push_back(i - 1);
        }
    }

    template <typename... Args> T* allocate(Args&&... args) {
        assert(!free_indices.empty());
        const uint32_t free_index = free_indices.back();
        free_indices.pop_back();
        void* addr = &data[free_index * sizeof(T)];
        T* ptr = ::new (addr) T(std::forward<Args>(args)...);
        return ptr;
    }
    void free(T* ptr) {
        assert(ptr);
        ptr->~T();
        const uint32_t index =
            (uintptr_t(ptr) - uintptr_t(data.data())) / sizeof(T);
        free_indices.push_back(index);
    }

    size_t getNumAllocations() { return SIZE - free_indices.size(); }
    uint32_t getIndex(T* ptr) {
        const uint32_t index =
            (uintptr_t(ptr) - uintptr_t(data.data())) / sizeof(T);
        return index;
    }
    void* getData() { return data.data(); }
    size_t getSize() { return SIZE; }
};

} // namespace Engine