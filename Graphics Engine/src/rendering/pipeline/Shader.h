#pragma once

#include <bitset>
#include <vector>

#include "../Direct3D11.h"

namespace Engine {
namespace Graphics {
// Shaders:
// Shaders are programs that can be invoked on the GPU. Currently,
// the engine supports the following shaders:
//    - Vertex Shader
//    - Pixel Shader
// Shaders can be bound to the graphics pipeline, and
// can have data passed into their constant buffers.
struct VertexShader {
    ID3D11VertexShader* shader;
    ID3D11InputLayout* layout;

    VertexShader(ID3D11VertexShader* shader, ID3D11InputLayout* layout);
    ~VertexShader();
};

struct PixelShader {
    ID3D11PixelShader* shader;

    PixelShader(ID3D11PixelShader* shader);
    ~PixelShader();
};

} // namespace Graphics
} // namespace Engine