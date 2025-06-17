#include "Shader.h"

#include <assert.h>
#include <cstring>

namespace Engine {
namespace Graphics {
VertexShader::VertexShader(ID3D11VertexShader* _shader,
                           ID3D11InputLayout* _layout) {
    shader = _shader;
    layout = _layout;
    layout_pin = 0;
}
VertexShader::~VertexShader() = default;

PixelShader::PixelShader(ID3D11PixelShader* _shader) { shader = _shader; }
PixelShader::~PixelShader() = default;

} // namespace Graphics
} // namespace Engine