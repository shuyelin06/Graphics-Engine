#pragma once

#include <cstdint>
#include <vector>

#include "DynamicBitset.h"

namespace Engine
{
// SlotMap:
// Simple implementation of a slot map.
// Insert and destroy elemetns in the slot map by handle. The slot map
// keeps the elements contiguous in memory.
// TODO: Add generational handles so we can do stale checks
using SlotMapHandle = uint32_t;
template <typename Element> class SlotMap
{
  private:
    static constexpr int kDefaultSlotMapSize = 16;

    std::vector<Element> data;
    std::vector<SlotMapHandle> freeList;
    DynamicBitset allocatedBitset;

  public:
    SlotMap() = default;
    ~SlotMap() = default;

    SlotMapHandle allocate()
    {
        // Resize if needed
        if (freeList.empty())
        {
            const size_t oldSize = data.size();
            const size_t newSize =
                data.empty() ? kDefaultSlotMapSize : data.size() * 2;

            data.resize(newSize);
            allocatedBitset.resize(newSize);

            // We do an offset by 1 to prevent integer underflowing
            for (size_t i = newSize; i > oldSize; i--)
            {
                freeList.push_back((SlotMapHandle) (i - 1));
            }
        }

        const SlotMapHandle freeHandle = freeList.back();
        freeList.pop_back();

        allocatedBitset.set(freeHandle, true);

        return freeHandle;
    }

    void destroy(SlotMapHandle handle)
    {
        freeList.push_back(handle);
        allocatedBitset.set(handle, false);
    }

    bool contains(SlotMapHandle handle) const
    {
        return allocatedBitset.test(handle);
    }

    Element& get(SlotMapHandle handle) { return data[handle]; }
    const Element& get(SlotMapHandle handle) const { return data[handle]; }

    const Element* getData() const { return data.data(); }
    const size_t getSize() const { return data.size(); }

    template <bool IsConst> class IteratorImpl
    {
      private:
        using ContainerPtr = std::
            conditional_t<IsConst, const SlotMap<Element>*, SlotMap<Element>*>;

        ContainerPtr slotMap;
        size_t index = 0;

        void findNextValid()
        {
            while (index < slotMap->data.size() &&
                   !slotMap->allocatedBitset.test(index))
            {
                index++;
            }
        }

      public:
        using iterator_category = std::forward_iterator_tag;
        using value_type = Element;
        using difference_type = std::ptrdiff_t;
        using pointer = std::conditional_t<IsConst, const Element*, Element*>;
        using reference = std::conditional_t<IsConst, const Element&, Element&>;

        IteratorImpl() = default;
        IteratorImpl(ContainerPtr container, size_t index)
            : slotMap(container)
            , index(index)
        {
            if (slotMap)
                findNextValid();
        }

        reference operator*() { return slotMap->data[index]; }
        pointer operator->() { return &slotMap->data[index]; }

        IteratorImpl& operator++()
        {
            ++index;
            findNextValid();
            return *this;
        }

        IteratorImpl operator++(int)
        {
            IteratorImpl tmp = *this;
            ++(*this);
            return tmp;
        }

        bool operator==(const IteratorImpl& other) const
        {
            return slotMap == other.slotMap && index == other.index;
        }

        bool operator!=(const IteratorImpl& other) const
        {
            return !(*this == other);
        }

        SlotMapHandle handle() const { return (SlotMapHandle) index; }
    };

    using Iterator = IteratorImpl<false>;
    using ConstIterator = IteratorImpl<true>;

    Iterator begin() { return Iterator(this, 0); }
    Iterator end() { return Iterator(this, data.size()); }

    ConstIterator begin() const { return ConstIterator(this, 0); }
    ConstIterator end() const { return ConstIterator(this, data.size()); }
    ConstIterator cbegin() const { return begin(); }
    ConstIterator cend() const { return end(); }
};

} // namespace Engine