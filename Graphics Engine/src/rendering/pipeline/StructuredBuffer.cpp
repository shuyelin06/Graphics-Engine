#include "StructuredBuffer.h"

#include "../Direct3D11.h"

namespace Engine {
namespace Graphics {
StructuredBuffer::StructuredBuffer() {
    elementSize = 0;
    numElements = 0;
    buffer = NULL;
    srv = NULL;
};
StructuredBuffer::~StructuredBuffer() {
    if (buffer)
        buffer->Release();
    if (srv)
        srv->Release();
}

void StructuredBuffer::initialize(ID3D11Device* device, size_t elementSize,
                                  size_t numElements) {
    this->elementSize = elementSize;
    this->numElements = numElements;

    // Create my buffer
    D3D11_BUFFER_DESC desc = {};
    desc.ByteWidth = elementSize * numElements;
    desc.Usage = D3D11_USAGE_DYNAMIC;
    desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    desc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
    desc.StructureByteStride = elementSize;

    device->CreateBuffer(&desc, NULL, &buffer);
    assert(buffer != NULL);

    // Create my shader resource view
    D3D11_SHADER_RESOURCE_VIEW_DESC srv_desc = {};
    srv_desc.Format = DXGI_FORMAT_UNKNOWN;
    srv_desc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
    srv_desc.Buffer.ElementOffset = 0;
    srv_desc.Buffer.NumElements = numElements;

    device->CreateShaderResourceView(buffer, &srv_desc, &srv);
    assert(srv != NULL);
}

// UploadData:
// Given a vector of data elements, uploads the data to the structured
// buffer. Takes the minimum of the two sizes to upload.
void StructuredBuffer::uploadData(ID3D11DeviceContext* context,
                                  const void* addr, unsigned int array_size) {
    const unsigned int num_elements = min(numElements, array_size);

    D3D11_MAPPED_SUBRESOURCE sr = {};
    context->Map(buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &sr);
    memcpy(sr.pData, addr, elementSize * num_elements);
    context->Unmap(buffer, 0);
}
} // namespace Graphics
} // namespace Engine