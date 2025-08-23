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

// Render Target Bind Flags:
// Flags for setting the render target.
enum TargetFlags {
    // Disables writes to the render target. Commonly used in the depth pass
    Target_Disabled = 0,
    // Enable the render target, and use the existing one.
    Target_UseExisting = 1,
    // Enable the render target, and swap the one in use. Commonly done
    // so that the shader can read data from the previous render target
    Target_SwapTarget = 2,
};

enum DepthStencilFlags {
    // Prevents the depth stencil from being bound
    Depth_Disabled = 0,
    // Enables the depth stencil and z-testing, but does not update the depth
    // value. The depth stencil can be read from in the shader while this flag
    // is set.
    Depth_TestNoWrite = 1,
    // Enables the depth stencil and z-testing, and updates the depth value
    // as well. The depth stencil can not be read from in the shader while set.
    Depth_TestAndWrite = 2,
    DepthFlagCount
};

enum BlendFlags {
    // Blending is done only off of the source alpha. For example, if srcA = 0.7, 
    // 70% of the color will be from the shader, and 30% from the render target
    Blend_SrcAlphaOnly = 0,
    // Blending is done off the source and destination alpha. For example, if srcA = 0.3,
    // destA = 0.7, 30% of the color will be from the shader, and 70% from the render target
    Blend_UseSrcAndDest = 1,
    BlendFlagCount,
    // ...
    Blend_Default = Blend_SrcAlphaOnly
};

// PipelineManager Class:
// Provides an interface for working with the 3D rendering pipeline.
// Uses D3D under the hood.
struct Mesh;
struct Texture;

class Pipeline {
  private:
    // D3D Interfaces
    HWND window;
    ID3D11Device* device;
    ID3D11DeviceContext* context;

    // Swapchain and Render Targets
    IDXGISwapChain* swapchain;
    D3D11_VIEWPORT viewport;

    Texture* screen_target;
    Texture* render_target_dest;
    Texture* render_target_src;
    Texture* depth_stencil;
    Texture* depth_stencil_copy;

    TargetFlags flag_target;
    DepthStencilFlags flag_depth;
    BlendFlags flag_blend;

    ID3D11DepthStencilState* depth_states[DepthFlagCount];
    ID3D11BlendState* blend_states[BlendFlagCount];

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

    void initializeTargets(HWND window);
    void initializeSamplers();

  public:
    Pipeline(HWND window);
    ~Pipeline();

    ID3D11Device* getDevice() const;
    ID3D11DeviceContext* getContext() const;
    Texture* getRenderTargetDest() const;
    Texture* getRenderTargetSrc() const;
    Texture* getDepthStencil() const;
    Texture* getDepthStencilCopy() const;

    // Prepare
    void prepare();

    // Shader Management
    bool bindVertexShader(const std::string& vs_name);
    bool bindPixelShader(const std::string& ps_name);

    // Render Target Management
    void bindRenderTarget(TargetFlags, DepthStencilFlags, BlendFlags);

    void bindInactiveTarget(int slot);
    void bindDepthStencil(int slot);

    // Shader Resource Management
    void bindSamplers();

    // Constant Buffer Management
    IConstantBuffer loadVertexCB(CBSlot slot);
    IConstantBuffer loadPixelCB(CBSlot slot);

    // Draw Calls. Set tri_end to -1 if you want it to draw all triangles
    // after tri_start.
    void drawMesh(const Mesh* mesh, int tri_start, int tri_end,
                  UINT instance_count);
    void drawPostProcessQuad();

    // Render to Screen
    void present();

  private:
    void swapActiveTarget();

#if defined(_DEBUG)
    // ImGui Display
    void imGuiInitialize(HWND window);

    void imGuiPrepare();
    void imGuiFinish();

    void imGuiShutdown();
#endif
};

} // namespace Graphics
} // namespace Engine