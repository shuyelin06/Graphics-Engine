#pragma once

#include "../Buffer.h"

#include "rendering/Direct3D11.h"

namespace Engine
{
namespace Graphics
{
class D3D11Buffer : public Buffer
{
  private:
    ID3D11Buffer* buffer;
    BufferType type;

  public:
    D3D11Buffer(ID3D11Device* device,
                BufferType type,
                size_t byteSize,
                const void* initData);
    ~D3D11Buffer();

    BufferType getBufferType() const override { return type; }
};

} // namespace Graphics
} // namespace Engine