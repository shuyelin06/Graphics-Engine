#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "D3D11Shader.h"
#include "rendering/Direct3D11.h"

#include "../VertexStreamIDs.h"

namespace Engine
{
namespace Graphics
{

struct ShaderConfig;

// ShaderManager Class:
// Manages shaders for the engine. Provides methods to create shaders,
// bind shaders, and validate shader inputs.
enum ShaderType
{
    Vertex,
    Pixel
};

class D3D11ShaderCompiler
{
  private:
    using VertexShaderStore =
        std::unordered_map<std::string_view,
                           std::unique_ptr<D3D11VertexShader>>;
    using PixelShaderStore =
        std::unordered_map<std::string_view, std::unique_ptr<D3D11PixelShader>>;
    ID3D11Device* device;

    VertexShaderStore vertex_shaders;
    PixelShaderStore pixel_shaders;

    // Used in compilation
    std::vector<D3D_SHADER_MACRO> shader_macros;

  public:
    D3D11ShaderCompiler(ID3D11Device* _device);
    ~D3D11ShaderCompiler();

    // Load and configure all of the shaders usable by the engine
    bool initializeShaders();

    // Access a shader by its respective enumerator slot.
    D3D11VertexShader* getVertexShader(const char* vs);
    D3D11PixelShader* getPixelShader(const char* ps);

  private:
    // Helper functions for compiling and building vertex and pixel shaders
    bool createVertexShader(const ShaderConfig& config,
                            VertexShaderStore& store);
    bool createPixelShader(const ShaderConfig& config, PixelShaderStore& store);

    ID3DBlob* compileShaderBlob(ShaderType type, const ShaderConfig& config);
};

} // namespace Graphics
} // namespace Engine