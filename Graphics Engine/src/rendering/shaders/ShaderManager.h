#pragma once

#include <unordered_map>
#include <vector>

#include "../Direct3D11.h"
#include "../core/VertexStreamIDs.h"
#include "Shader.h"

namespace Engine {
namespace Graphics {

struct ShaderConfig;

// ShaderManager Class:
// Manages shaders for the engine. Provides methods to create shaders,
// bind shaders, and validate shader inputs.
enum ShaderType { Vertex, Pixel };

class ShaderManager {
  private:
    ID3D11Device* device;

    std::unordered_map<std::string, VertexShader*> vertex_shaders;
    std::unordered_map<std::string, PixelShader*> pixel_shaders;

    // Common variables for initialization
    std::vector<VertexDataStream> input_layout_arr;
    std::vector<std::string> pins_arr;

    // Used in compilation
    std::vector<D3D_SHADER_MACRO> shader_macros;

  public:
    ShaderManager(ID3D11Device* _device);
    ~ShaderManager();

    // Load and configure all of the shaders usable by the engine
    void initializeShaders();

    // Access a shader by its respective enumerator slot.
    VertexShader* getVertexShader(const std::string& name);
    PixelShader* getPixelShader(const std::string& name);

  private:
    // Helper functions for compiling and building vertex and pixel shaders
    void createVertexShader(const ShaderConfig& config);
    void createPixelShader(const ShaderConfig& config);

    ID3DBlob* compileShaderBlob(ShaderType type, const ShaderConfig& config);
};

} // namespace Graphics
} // namespace Engine