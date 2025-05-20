#pragma once

#include "ConstantBuffer.h"
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

    // Swapchain and Render Targets

    // Constant Buffer Handles
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

    // Render Target Management
    void swapActiveRenderTarget();
    void bindActiveRenderTarget();

    // Draw Calls
    void drawMesh(VertexDataPin vertex_layout);
    void drawPostProcessQuad();

    // Render to Screen
    void present();

  private:
    void initializeTargets(HWND window);
};

} // namespace Graphics
} // namespace Engine