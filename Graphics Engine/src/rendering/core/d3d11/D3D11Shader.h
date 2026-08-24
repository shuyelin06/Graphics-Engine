#pragma once

#include "rendering/Direct3D11.h"

namespace Engine
{
namespace Graphics
{
// Shaders:
// Shaders are programs that can be invoked on the GPU. Currently,
// the engine supports the following shaders:
//    - Vertex Shader
//    - Pixel Shader
// Shaders can be bound to the graphics pipeline, and
// can have data passed into their constant buffers.
struct D3D11VertexShader
{
    ID3D11VertexShader* shader = nullptr;
    ID3D11InputLayout* layout = nullptr;

    D3D11VertexShader() = default;
    D3D11VertexShader(const D3D11VertexShader& other);
    D3D11VertexShader(D3D11VertexShader&& other);
    D3D11VertexShader(ID3D11VertexShader* shader, ID3D11InputLayout* layout);
    ~D3D11VertexShader();

    void operator=(const D3D11VertexShader& other)
    {
        shader = other.shader;
        layout = other.layout;
    }
};

struct D3D11PixelShader
{
    ID3D11PixelShader* shader = nullptr;

    D3D11PixelShader() = default;
    D3D11PixelShader(const D3D11PixelShader& other);
    D3D11PixelShader(D3D11PixelShader&& other);
    D3D11PixelShader(ID3D11PixelShader* shader);
    ~D3D11PixelShader();

    void operator=(const D3D11PixelShader& other) { shader = other.shader; }
};

} // namespace Graphics
} // namespace Engine