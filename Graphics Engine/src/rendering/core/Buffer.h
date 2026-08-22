#pragma once

#include <cstdint>

namespace Engine
{
namespace Graphics
{
enum class BufferType : uint8_t
{
    Index = 0,
    Vertex = 1,
};

class Buffer
{
  public:
    Buffer() {};
    virtual ~Buffer() {};

    virtual BufferType getBufferType() const = 0;
};

} // namespace Graphics
} // namespace Engine