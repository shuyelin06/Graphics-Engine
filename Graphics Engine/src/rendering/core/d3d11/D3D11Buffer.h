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
    size_t byteSize;
    bool dynamic = false;

  public:
    D3D11Buffer(ID3D11Device* device,
                BufferType type,
                size_t byteSize,
                const void* initData,
                bool dynamic);
    ~D3D11Buffer();

    ID3D11Buffer* getBuffer() const { return buffer; }
    BufferType getBufferType() const override { return type; }

    void upload(ID3D11DeviceContext* context, const void* src, size_t bytes);
};

} // namespace Graphics
} // namespace Engine