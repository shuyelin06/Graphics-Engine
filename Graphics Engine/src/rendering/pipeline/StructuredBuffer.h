#pragma once

#include <assert.h>

struct ID3D11Device;
struct ID3D11DeviceContext;
struct ID3D11Buffer;
struct ID3D11ShaderResourceView;

namespace Engine {
namespace Graphics {
// StructuredBuffer Class:
// An interface for working with structured buffers in this engine.
class StructuredBuffer {
  private:
    friend class Pipeline;

    ID3D11Buffer* buffer;
    ID3D11ShaderResourceView* srv;

    size_t elementSize;
    size_t numElements;

  public:
    StructuredBuffer();
    ~StructuredBuffer();

    void initialize(ID3D11Device* device, size_t elementSize,
                    size_t numElements);

    // UploadData:
    // Given a vector of data elements, uploads the data to the structured
    // buffer. Takes the minimum of the two sizes to upload.
    void uploadData(ID3D11DeviceContext* context, const void* addr,
                    unsigned int array_size);
};

} // namespace Graphics
} // namespace Engine