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
}
D3D11VertexShader::D3D11VertexShader(const D3D11VertexShader& other)
    : shader(other.shader)
    , layout(other.layout)
{
}
D3D11VertexShader::D3D11VertexShader(D3D11VertexShader&& other)
    : shader(other.shader)
    , layout(other.layout)
{
    other.shader = nullptr;
    other.layout = nullptr;
}
D3D11VertexShader::~D3D11VertexShader()
{
    if (shader)
        shader->Release();
    if (layout)
        layout->Release();
}

D3D11PixelShader::D3D11PixelShader(ID3D11PixelShader* _shader)
{
    shader = _shader;
}
D3D11PixelShader::D3D11PixelShader(const D3D11PixelShader& other)
    : shader(other.shader)
{
}
D3D11PixelShader::D3D11PixelShader(D3D11PixelShader&& other)
    : shader(other.shader)
{
    other.shader = nullptr;
}
D3D11PixelShader::~D3D11PixelShader()
{
    if (shader)
        shader->Release();
}

} // namespace Graphics
} // namespace Engine