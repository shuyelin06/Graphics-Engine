#pragma once

#include "ConstantBuffer.h"
#include "Shader.h"
#include "ShaderManager.h"

constexpr int SAMPLER_COUNT = 4;

#define INDEX_LIST_START 0
#define INDEX_LIST_END -1

namespace Engine {
namespace Graphics {
// SamplerSlot Enum:
// References the sampler slots in the pipeline.
// Most of the samplers are not to be rebound, as they are
// commonly used.
enum SamplerSlot {
    Point = 0,
    Shadow = 1,
    // Linear = 2,
    // Anisotrophic = 3,
    // Note: Additional samplers can be added here
    SamplerCount
};

// PipelineManager Class:
// Provides an interface for working with the 3D rendering pipeline.
// Uses D3D under the hood.
struct Mesh;
struct Texture;

class PipelineManager {
  private:
    // D3D Interfaces
    ID3D11Device* device;
    ID3D11DeviceContext* context;

    // Swapchain and Render Targets
    // ...

    // Samplers
    ID3D11SamplerState* samplers[SAMPLER_COUNT];

    // Bound Vertex / Index Buffer
    const void* active_pool_addr;
    ID3D11Buffer* vb_buffers[BINDABLE_STREAM_COUNT];
    UINT vb_strides[BINDABLE_STREAM_COUNT];
    UINT vb_offsets[BINDABLE_STREAM_COUNT];

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

    // Shader Resource Management
    void bindSamplers();

    // Constant Buffer Management
    IConstantBuffer loadVertexCB(CBSlot slot);
    IConstantBuffer loadPixelCB(CBSlot slot);

    // Render Target Management
    void swapActiveRenderTarget();
    void bindActiveRenderTarget();

    // Draw Calls. Set tri_end to -1 if you want it to draw all triangles
    // after tri_start.
    void drawMesh(const Mesh* mesh, int tri_start, int tri_end,
                  UINT instance_count);
    void drawPostProcessQuad();

    // Render to Screen
    void present();

  private:
    void initializeTargets(HWND window);
    void initializeSamplers();
};

} // namespace Graphics
} // namespace Engine