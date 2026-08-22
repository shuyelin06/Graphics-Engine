#pragma once

#include "rendering/Direct3D11.h"

#include "rendering/core/VertexStreamIDs.h"

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
    ID3D11VertexShader* shader;
    ID3D11InputLayout* layout;

    // Each bit tells us if the VertexDataStream
    // at that bit (slot) position is to be used
    // (see VertexStreamIDs.h)
    VertexLayout vertexLayout;

    D3D11VertexShader(ID3D11VertexShader* shader, ID3D11InputLayout* layout);
    ~D3D11VertexShader();
};

struct D3D11PixelShader
{
    ID3D11PixelShader* shader;

    D3D11PixelShader(ID3D11PixelShader* shader);
    ~D3D11PixelShader();
};

} // namespace Graphics
} // namespace Engine