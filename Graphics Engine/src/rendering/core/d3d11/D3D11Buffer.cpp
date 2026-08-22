#include "D3D11Buffer.h"

#include <assert.h>

namespace Engine
{
namespace Graphics
{
static UINT bufferTypeBindFlag(BufferType type)
{
    switch (type)
    {
    case BufferType::Index:
        return D3D11_BIND_INDEX_BUFFER;
    case BufferType::Vertex:
        return D3D11_BIND_VERTEX_BUFFER;
    default:
        assert(false); // Unsupported
    }
}

D3D11Buffer::D3D11Buffer(ID3D11Device* device,
                         BufferType type,
                         size_t byteSize,
                         const void* initData,
                         bool dynamic)
    : type(type)
    , byteSize(byteSize)
    , dynamic(dynamic)
{
    D3D11_BUFFER_DESC desc = {};
    desc.ByteWidth = byteSize;
    desc.Usage = dynamic ? D3D11_USAGE_DYNAMIC : D3D11_USAGE_DEFAULT;
    desc.BindFlags = bufferTypeBindFlag(type);
    desc.CPUAccessFlags = dynamic ? D3D11_CPU_ACCESS_WRITE : 0;

    if (initData != nullptr)
    {
        D3D11_SUBRESOURCE_DATA srData = {};
        srData.pSysMem = initData;
        // SysMemPitch, SysMemSlicePitch has no meaning for buffers. Only used for textures
        HRESULT result = device->CreateBuffer(&desc, &srData, &buffer);
        assert(SUCCEEDED(result));
    }
    else
    {
        HRESULT result = device->CreateBuffer(&desc, NULL, &buffer);
        assert(SUCCEEDED(result));
    }
}

D3D11Buffer::~D3D11Buffer()
{
    if (buffer)
    {
        buffer->Release();
    }
}

void D3D11Buffer::upload(ID3D11DeviceContext* context,
                         const void* src,
                         size_t bytes)
{
    assert(bytes <= byteSize);
    if (dynamic)
    {
        D3D11_MAPPED_SUBRESOURCE srData = {};
        HRESULT result =
            context->Map(buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &srData);
        assert(SUCCEEDED(result));
        memcpy(srData.pData, src, bytes);
        context->Unmap(buffer, 0);
    }
    else
    {
        context->UpdateSubresource(buffer, 0, NULL, src, 0, 0);
    }
}

} // namespace Graphics
} // namespace Engine