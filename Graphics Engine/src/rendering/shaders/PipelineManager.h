#pragma once

#include "Shader.h"
#include "ShaderManager.h"

namespace Engine {
namespace Graphics {
// PipelineManager Class:
// Provides an interface for working with the 3D rendering pipeline.
// Uses D3D under the hood.
class PipelineManager {
  private:
    // D3D Interfaces
    ID3D11Device* device;
    ID3D11DeviceContext* context;

    // Shader Manager
    ShaderManager* shader_manager;

    // Shared constant buffer handles.
    CBHandle* vcb_handles[CBCOUNT];
    CBHandle* pcb_handles[CBCOUNT];

    // Active Shaders
    VertexShader* vs_active;
    PixelShader* ps_active;

  public:
    PipelineManager(ID3D11Device* device, ID3D11DeviceContext* context);

    // Accessors
    CBHandle* getVertexCB(CBSlot slot) const;
    CBHandle* getPixelCB(CBSlot slot) const;

    // Pipeline Binding
    bool bindVertexShader(const std::string& vs_name);
    bool bindPixelShader(const std::string& ps_name);

    void bindVertexCB(CBSlot slot);
    void bindPixelCB(CBSlot slot);

  private:
    void updateCBData(CBHandle* handle);
};

} // namespace Graphics
} // namespace Engine