#pragma once

#include "../Direct3D11.h"
#include <assert.h>
#include <vector>

namespace Engine {
namespace Graphics {
// Constant Buffers:
// Constant buffers are arrays of data which can be bound to a shader,
// and cannot be changed for that invocation of the program.
// Handling of any given constant buffer for a shader will be done by
// a corresponding CBHandle object.
enum CBSlot {
    CB0 = 0,
    CB1 = 1,
    CB2 = 2,
    CB3 = 3,
    CBCOUNT // Enum trick to track the # of slots supported
};

enum CBDataFormat {
    INT = 4,
    FLOAT = 4,
    FLOAT2 = 8,
    FLOAT3 = 12,
    FLOAT4 = 16,
    FLOAT4X4 = 64
};

// TODO: Make constant buffer owned by resourceManager
// Resource Manager processes cbuffer invalidations (really, all
// resource uploads to GPU) before RenderManager kicks off.
template <typename DataType> class ConstantBuffer {
  private:
    DataType data;
    ID3D11Buffer* gpuResource;

    // Flag that tracks whether or not the buffer has been
    // invalidated (had data uploaded to it) this frame, to check against
    // double invalidates.
    bool invalidated;

  public:
    ConstantBuffer();
    ~ConstantBuffer();

    void loadData(DataType data) {
        assert(!invalidated);
        this->data = data;
        invalidated = true;
    }

    // Clear data stored within the constant buffer.
    void clearData();
};

class ConstantBuffer_DEPRECATED {
    friend class IConstantBuffer;

  private:
    std::vector<uint8_t> data;
    ID3D11Buffer* resource;

    size_t buffer_size;
    size_t max_size;

  public:
    ConstantBuffer_DEPRECATED();
    ConstantBuffer_DEPRECATED(size_t max_size);
    ~ConstantBuffer_DEPRECATED();

    // Returns the number of bytes currently loaded into the constant buffer.
    unsigned int byteSize();

    // Load data into the constant buffer. Note that validation and padding
    // is not done on the input.
    // To pass in padding (0's), pass in a null pointer.
    void loadData(const void* dataPtr, size_t byteSize);

    // Clear data stored within the constant buffer.
    void clearData();
};

// IConstantBuffer Class:
// Provides automatic clearing and binding of constant
// buffer data on construction and destruction.
enum IBufferType { CBVertex, CBPixel };
class IConstantBuffer {
  private:
    ID3D11Device* device;
    ID3D11DeviceContext* context;

    ConstantBuffer_DEPRECATED* cb;
    CBSlot slot;
    IBufferType type;

  public:
    IConstantBuffer(ConstantBuffer_DEPRECATED* cb, CBSlot slot,
                    IBufferType type, ID3D11Device* device,
                    ID3D11DeviceContext*);
    ~IConstantBuffer();

    void loadData(const void* dataPtr, size_t byteSize);

  private:
    // Upload and bind the CB to the pipeline
    void bindCB();
};

} // namespace Graphics
} // namespace Engine