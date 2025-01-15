#pragma once

#include <map>
#include <string>
#include <vector>

#include <assert.h>

typedef unsigned int UINT;

namespace Engine {
namespace Utility {
// IndexMap Class:
// A container that associates its contents with unique indices,
// providing fast indexing while maintaining fairly-efficient searching
// if indices with respect to a unique string identifier.
template <typename T> class IndexMap {
  private:
    // Stores the actual data of the IndexMap
    std::vector<T> elements;

    // Maps string identifiers to their indices
    std::map<std::string, UINT> string_map;

  public:
    IndexMap() = default;
    ~IndexMap() = default;

    // AddElement:
    // Adds an element to the index map, and returns an index identifier to
    // the element in the map.
    int addElement(const std::string& id, const T& element) {
        if (string_map.contains(id))
            return -1;
        else {
            int index = elements.size();
            elements.push_back(element);
            string_map[id] = index;

            return index;
        }
    }
    
    // GetElement:
    // Get elements in the container by index or string identifier
    T getElement(const UINT index) {
        assert(0 <= index && index < elements.size());
        return elements[index];
    }
    T getElement(const std::string& id) {
        assert(string_map.contains(id));

        UINT index = string_map[id];
        return elements[index];
    }
};
// Container that provides fast-indexing and flexibility
// with adding new elements with, as well as
// semi-fast
} // namespace Utility
} // namespace Engine