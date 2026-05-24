#pragma once

#include <assert.h>
#include <vector>

namespace Engine {
// ArenaAllocator:
// Simple implementation of an arena allocator.
// This allocates a large chunk of memory, which is used for the arena's entire
// lifetime.
// On "allocation" and "free", this allocator returns pointers to regions of
// this block of memory.
template <typename T, size_t SIZE> class PoolAllocator {
    std::vector<T> data;
    std::vector<uint16_t> free_indices;

  public:
    PoolAllocator() {
        data.resize(SIZE);
        // Add to free indices in descending order, so that when we pop we use
        // free indices from 0 --> size (better for caching)s
        for (uint16_t i = SIZE; i > 0; i--) {
            free_indices.push_back(i - 1);
        }
    }

    T* allocate() {
        assert(!free_indices.empty());
        const uint32_t free_index = free_indices.back();
        free_indices.pop_back();
        T* ptr = &data[free_index];
        return ptr;
    }
    void free(T* ptr) {
        const uint32_t index =
            (uintptr_t(ptr) - uintptr_t(data.data())) / sizeof(T);
        free_indices.push_back(index);
    }

    uint32_t getIndex(T* ptr) {
        const uint32_t index =
            (uintptr_t(ptr) - uintptr_t(data.data())) / sizeof(T);
        return index;
    }
    T* getData() { return data.data(); }
    size_t getSize() { return SIZE; }
};

} // namespace Engine