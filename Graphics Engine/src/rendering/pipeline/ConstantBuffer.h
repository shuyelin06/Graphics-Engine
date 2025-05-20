#pragma once

#include <vector>
#include "../Direct3D11.h"

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

class CBHandle {
    friend class IConstantBuffer;

  private:
    std::vector<uint8_t> data;
    ID3D11Buffer* resource;

    UINT buffer_size;

  public:
    CBHandle();
    ~CBHandle();

    // Returns the number of bytes currently loaded into the constant buffer.
    unsigned int byteSize();

    // Load data into the constant buffer. Note that validation and padding
    // is not done on the input.
    // To pass in padding (0's), pass in a null pointer.
    void loadData(const void* dataPtr, CBDataFormat format);

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

    CBHandle* cb;
    CBSlot slot;
    IBufferType type;

  public:
    IConstantBuffer(CBHandle* cb, CBSlot slot, IBufferType type,
                    ID3D11Device* device, ID3D11DeviceContext*);
    ~IConstantBuffer();

    void loadData(const void* dataPtr, CBDataFormat dataFormat);

  private:
    // Upload and bind the CB to the pipeline
    void bindCB();
};

} // namespace Graphics
} // namespace Engine