#include "Shader.h"

#include <assert.h>
#include <cstring>

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

VertexShader::VertexShader(ID3D11VertexShader* _shader,
                           ID3D11InputLayout* _layout) {
    shader = _shader;
    layout = _layout;
}
VertexShader::~VertexShader() = default;

PixelShader::PixelShader(ID3D11PixelShader* _shader) { shader = _shader; }
PixelShader::~PixelShader() = default;

} // namespace Graphics
} // namespace Engine