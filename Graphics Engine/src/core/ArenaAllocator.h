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
template <typename T, size_t SIZE> class ArenaAllocator {
    std::vector<T> data;
    std::vector<uint16_t> free_indices;

  public:
    ArenaAllocator() {
        data.resize(SIZE);
        for (uint16_t i = 0; i < SIZE; i++) {
            free_indices.push_back(i);
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
};

} // namespace Engine