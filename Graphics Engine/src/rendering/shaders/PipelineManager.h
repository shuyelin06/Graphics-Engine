#pragma once

#include "Shader.h"
#include "ShaderManager.h"

namespace Engine {
namespace Graphics {
class PipelineManager;

// IConstantBuffer Class:
// Provides automatic clearing and binding of constant
// buffer data on construction and destruction.
enum IBufferType { CBVertex, CBPixel };
class IConstantBuffer {
  private:
    CBHandle* cb_handle;

    IBufferType type;
    PipelineManager* pipeline;
    CBSlot slot;

  public:
    IConstantBuffer(PipelineManager* pipeline, IBufferType type, CBSlot slot);
    ~IConstantBuffer();

    void loadData(const void* dataPtr, CBDataFormat dataFormat);
};

// PipelineManager Class:
// Provides an interface for working with the 3D rendering pipeline.
// Uses D3D under the hood.
class PipelineManager {
  private:
    // D3D Interfaces
    ID3D11Device* device;
    ID3D11DeviceContext* context;

    // Shared constant buffer handles.
    CBHandle* vcb_handles[CBCOUNT];
    CBHandle* pcb_handles[CBCOUNT];

    // Active Shaders
    ShaderManager* shader_manager;
    VertexShader* vs_active;
    PixelShader* ps_active;

    // Post Processing
    ID3D11Buffer* postprocess_quad;

  public:
    PipelineManager(ID3D11Device* device, ID3D11DeviceContext* context);
    ~PipelineManager();

    // Shader Management
    bool bindVertexShader(const std::string& vs_name);
    bool bindPixelShader(const std::string& ps_name);

    // Constant Buffer Management
    IConstantBuffer loadVertexCB(CBSlot slot);
    IConstantBuffer loadPixelCB(CBSlot slot);

    // Draw Calls
    void drawMesh(VertexDataPin vertex_layout);
    void drawPostProcessQuad();

    // (START TO DEPRECATE) Accessors
    CBHandle* getVertexCB(CBSlot slot) const;
    CBHandle* getPixelCB(CBSlot slot) const;

    void bindVertexCB(CBSlot slot);
    void bindPixelCB(CBSlot slot);

  private:
    void updateCBData(CBHandle* handle);
};

} // namespace Graphics
} // namespace Engine