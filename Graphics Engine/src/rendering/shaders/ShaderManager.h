#pragma once

#include <unordered_map>

#include "Shader.h"
#include "../Direct3D11.h"
#include "../_VertexStreamIDs_.h"

namespace Engine {
namespace Graphics {

// ShaderManager Class:
// Manages shaders for the engine. Provides methods to create shaders,
// bind shaders, and validate shader inputs.
class ShaderManager {
  private:
    ID3D11Device* device;

    std::unordered_map<std::string, VertexShader*> vertex_shaders;
    std::unordered_map<std::string, PixelShader*> pixel_shaders;

  public:
    ShaderManager(ID3D11Device* _device);
    ~ShaderManager();

    // Load and configure all of the shaders usable by the engine
    void initialize();

    // Access a shader by its respective enumerator slot.
    VertexShader* getVertexShader(const std::string& name);
    PixelShader* getPixelShader(const std::string& name);

  private:
    // Helper functions for compiling and building vertex and pixel shaders
    VertexShader* createVertexShader(const std::string& filename,
                                     const char* entrypoint,
                                     VertexDataStream* input_data,
                                     UINT input_data_size);
    PixelShader* createPixelShader(const std::string& filename,
                                   const char* entrypoint);
};

} // namespace Graphics
} // namespace Engine