#include "include/core/DynamicBitset.h"

namespace Engine
{
void DynamicBitset::resize(size_t bits)
{
    const size_t byteSize = bits / 8 + 1;
    data.resize(byteSize);
}

bool DynamicBitset::test(size_t bit) const
{
    const size_t byte = bit / 8;
    const uint8_t localBit = bit % 8;
    return (data[byte] >> localBit) & 1;
}

void DynamicBitset::set(size_t bit, bool value)
{
    const size_t byte = bit / 8;
    const uint8_t localBit = bit % 8;
    if (value)
        data[byte] |= 1 << localBit;
    else
        data[byte] ^= 1 << localBit;
}

void DynamicBitset::reset() { memset(data.data(), data.size(), 0); }

size_t DynamicBitset::memorySize() { return data.size(); }

} // namespace Engine