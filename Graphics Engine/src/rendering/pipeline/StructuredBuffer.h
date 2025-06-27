#pragma once

#include "../Direct3D11.h"
#include <assert.h>

namespace Engine {
namespace Graphics {
// StructuredBuffer Class:
// An interface for working with structured buffers in this engine.
template <typename T> class StructuredBuffer {
  private:
    ID3D11Buffer* buffer;
    ID3D11ShaderResourceView* srv;

    unsigned int size;

  public:
    StructuredBuffer() {
        size = 0;
        buffer = NULL;
        srv = NULL;
    };

    void initialize(ID3D11Device* device, unsigned int num_elements) {
        size = num_elements;

        // Create my buffer
        D3D11_BUFFER_DESC desc = {};
        desc.ByteWidth = size * sizeof(T);
        desc.Usage = D3D11_USAGE_DYNAMIC;
        desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
        desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
        desc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
        desc.StructureByteStride = sizeof(T);

        device->CreateBuffer(&desc, NULL, &buffer);
        assert(buffer != NULL);

        // Create my shader resource view
        D3D11_SHADER_RESOURCE_VIEW_DESC srv_desc = {};
        srv_desc.Format = DXGI_FORMAT_UNKNOWN;
        srv_desc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
        srv_desc.Buffer.ElementOffset = 0;
        srv_desc.Buffer.NumElements = size;

        device->CreateShaderResourceView(buffer, &srv_desc, &srv);
        assert(srv != NULL);
    }

    // UploadData:
    // Given a vector of data elements, uploads the data to the structured
    // buffer. Takes the minimum of the two sizes to upload.
    void uploadData(ID3D11DeviceContext* context, void* addr,
                    unsigned int array_size) {
        const unsigned int num_elements = min(size, array_size);

        D3D11_MAPPED_SUBRESOURCE sr = {};
        context->Map(buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &sr);
        memcpy(sr.pData, addr, sizeof(T) * num_elements);
        context->Unmap(buffer, 0);
    }

    // BindBuffer:
    // Binds ths buffer to the GPU.
    void VSBindResource(ID3D11DeviceContext* context, unsigned int slot) const {
        context->VSSetShaderResources(slot, 1, &srv);
    }
    void PSBindResource(ID3D11DeviceContext* context, unsigned int slot) const {
        context->PSSetShaderResources(slot, 1, &srv);
    }
};

} // namespace Graphics
} // namespace Engine