#include "D3D11Shader.h"

#include <assert.h>
#include <cstring>

namespace Engine
{
namespace Graphics
{
D3D11VertexShader::D3D11VertexShader(ID3D11VertexShader* _shader,
                           ID3D11InputLayout* _layout)
{
    shader = _shader;
    layout = _layout;
    vertexLayout = VertexLayout();
}
D3D11VertexShader::~D3D11VertexShader() = default;

D3D11PixelShader::D3D11PixelShader(ID3D11PixelShader* _shader)
{
    shader = _shader;
}
D3D11PixelShader::~D3D11PixelShader() = default;

} // namespace Graphics
} // namespace Engine