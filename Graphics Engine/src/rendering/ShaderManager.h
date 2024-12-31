#pragma once

#include "Direct3D11.h"
#include "_VertexStreamIDs_.h"

#include "Shader.h"

namespace Engine {
namespace Graphics {

// Index references to available vertex and pixel
// shaders. We use indices so that these
// shaders can be accessed efficiently
enum VSSlot {
    VSShadowMap = 0,
    VSDebugPoint = 1,
    VSDebugLine = 2,
    VSDefault = 3,
    VSShadow = 4,
    VSTerrain = 5,
    VSCount
};

enum PSSlot {
    PSShadowMap = 0,
    PSDebugPoint = 1,
    PSDebugLine = 2,
    PSDefault = 3,
    PSShadow = 4,
    PSTerrain = 5,
    PSCount
};

// ShaderManager Class:
// Manages shaders for the engine. Provides methods to create shaders,
// bind shaders, and validate shader inputs.
class ShaderManager {
  private:
    ID3D11Device* device;

    std::vector<VertexShader*> vertexShaders;
    std::vector<PixelShader*> pixelShaders;

  public:
    ShaderManager(ID3D11Device* _device);
    ~ShaderManager();

    // Load and configure all of the shaders usable by the engine
    void initialize();

    // Access a shader by its respective enumerator slot.
    VertexShader* getVertexShader(VSSlot slot);
    PixelShader* getPixelShader(PSSlot slot);

  private:
    // Helper functions for compiling and building vertex and pixel shaders
    VertexShader* createVertexShader(const std::string filename,
                                     const char* entrypoint,
                                     VertexDataStream input_data[],
                                     UINT input_data_size);
    PixelShader* createPixelShader(const std::string filename,
                                   const char* entrypoint);
};
} // namespace Graphics
} // namespace Engine