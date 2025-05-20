#include "ConstantBuffer.h"

#include <assert.h>

namespace Engine {
namespace Graphics {
CBHandle::CBHandle() : data(0), resource(nullptr) {}
CBHandle::~CBHandle() = default;

unsigned int CBHandle::byteSize() { return data.size(); }

void CBHandle::loadData(const void* dataPtr, CBDataFormat dataFormat) {
    // Convert our data into a character array, and read the number of bytes
    // specified by the CBDataFormat into our constant buffer.
    const char* charData = static_cast<const char*>(dataPtr);
    const int numBytes = dataFormat;

    if (dataPtr != nullptr) {
        char value;

        for (int i = 0; i < numBytes; i++) {
            std::memcpy(&value, charData + i, sizeof(char));
            data.push_back(value);
        }
    } else {
        for (int i = 0; i < numBytes; i++)
            data.push_back(0);
    }
}

void CBHandle::clearData() { data.clear(); }

IConstantBuffer::IConstantBuffer(CBHandle* _cb, CBSlot _slot, IBufferType _type,
                                 ID3D11Device* _device,
                                 ID3D11DeviceContext* _context) {
    device = _device;
    slot = _slot;
    context = _context;

    cb = _cb;
    type = _type;

    cb->clearData();
}

IConstantBuffer::~IConstantBuffer() { bindCB(); }

void IConstantBuffer::loadData(const void* dataPtr, CBDataFormat dataFormat) {
    cb->loadData(dataPtr, dataFormat);
}

void IConstantBuffer::bindCB() {
    // Do nothing if CB has nothing
    if (cb->byteSize() == 0)
        return;

    // If the buffer resource has never been created before, or the current
    // resource is too small for mapping / unmapping, create a new one.
    if (cb->resource == nullptr) {
        // Create buffer to allow dynamic usage, i.e. accessible by
        // GPU read and CPU write. We opt for this usage so that we can update
        // the resource on the fly when needed.
        D3D11_BUFFER_DESC buff_desc = {};
        buff_desc.ByteWidth = 65536;
        buff_desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        buff_desc.Usage = D3D11_USAGE_DYNAMIC;
        buff_desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

        // Create buffer
        HRESULT result =
            device->CreateBuffer(&buff_desc, NULL, &(cb->resource));
        assert(SUCCEEDED(result));
    }

    // Perform resource renaming to update buffer data
    // Disable GPU access to data and obtain the my constant buffer resource
    D3D11_MAPPED_SUBRESOURCE mapped_resource = {0};
    context->Map(cb->resource, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped_resource);

    // Update the data in the resource
    memcpy(mapped_resource.pData, cb->data.data(), cb->byteSize());

    // Reenable GPU access to data
    context->Unmap(cb->resource, 0);
    cb->buffer_size = cb->byteSize();

    // Bind to the pipeline
    if (type == CBVertex)
        context->VSSetConstantBuffers(slot, 1, &cb->resource);
    else if (type == CBPixel)
        context->PSSetConstantBuffers(slot, 1, &cb->resource);
}

} // namespace Graphics
} // namespace Engine