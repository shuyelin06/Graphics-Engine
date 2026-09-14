#pragma once

#include <cstdint>
#include <vector>

namespace Engine
{
class DynamicBitset
{
  private:
    std::vector<uint8_t> data;

  public:
    DynamicBitset() = default;

    void resize(size_t bits);

    bool test(size_t bit) const;
    void set(size_t bit, bool value);
    void reset();

    size_t memorySize();
};

} // namespace Engine