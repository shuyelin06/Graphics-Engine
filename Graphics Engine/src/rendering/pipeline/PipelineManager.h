#pragma once

#include <cstdint>

#include "../Direct3D11.h"
#include "rendering/core/Device.h"
#include "rendering/core/RenderSettings.h"
#include "rendering/core/Texture.h"

#include "Shader.h"
#include "ShaderManager.h"
#include "StructuredBuffer.h"

#include "EnumTypes.h"

#define INDEX_LIST_START 0
#define INDEX_LIST_END -1

namespace Engine
{
namespace Graphics
{
// VertexTopology Enum:
// Specifies how the vertices are arranged.
enum class VertexTopology : uint8_t
{
    TriangleList = 0,
    LineList = 1,
    _Count_,
};

// Render Target Bind Flags:
// Flags for setting the render target.
enum TargetFlags
{
    // Disables writes to the render target. Commonly used in the depth pass
    Target_Disabled = 0,
    // Enable the render target, and use the existing one.
    Target_UseExisting = 1,
    // Enable the render target, and swap the one in use. Commonly done
    // so that the shader can read data from the previous render target
    Target_SwapTarget = 2,
};

enum DepthStencilFlags
{
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

enum BlendFlags
{
    // Blending is done only off of the source alpha. For example, if srcA =
    // 0.7,
    // 70% of the color will be from the shader, and 30% from the render target
    Blend_SrcAlphaOnly = 0,
    // Blending is done off the source and destination alpha. For example, if
    // srcA = 0.3,
    // destA = 0.7, 30% of the color will be from the shader, and 70% from the
    // render target
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

class Pipeline
{
  private:
    // D3D Interfaces
    HWND window;
    std::unique_ptr<Device> device;
    std::unique_ptr<DeviceContext> context;

    // Swapchain and Render Targets
    D3D11_VIEWPORT viewport;

    std::shared_ptr<Texture> render_target_dest = nullptr;
    std::shared_ptr<Texture> render_target_src = nullptr;
    std::shared_ptr<Texture> depth_stencil = nullptr;

    TargetFlags flag_target;
    DepthStencilFlags flag_depth;
    BlendFlags flag_blend;

    ID3D11DepthStencilState* depth_states[DepthFlagCount];
    ID3D11BlendState* blend_states[BlendFlagCount];

    // Samplers
    ID3D11SamplerState* samplers[SamplerType::SamplerCount];

    // Bound Vertex / Index Buffer
    const void* active_pool_addr;
    ID3D11Buffer* vb_buffers[BINDABLE_STREAM_COUNT];
    UINT vb_strides[BINDABLE_STREAM_COUNT];
    UINT vb_offsets[BINDABLE_STREAM_COUNT];

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

    Device* getDevice() const;
    DeviceContext* getContext() const;
    Texture* getRenderTargetDest() const;
    Texture* getRenderTargetSrc() const;
    Texture* getDepthStencil() const;

    // Prepare
    void beginFrame(const uint64_t frame);

    // Vertex Technique API
    void bindVertexShader(const std::string& vs_name);
    void setVertexTopology(VertexTopology topology);

    void bindVertexSB(const StructuredBuffer& sb, unsigned int slot)
    {
        context->getContext()->VSSetShaderResources(slot, 1, &sb.srv);
    }
    void bindVertexTexture(uint8_t slot,
                           const Texture& texture,
                           SamplerType samplerType);

    // Pixel Technique API
    void bindRenderTarget(Texture* renderTarget,
                          Texture* depthStencil,
                          DepthStencilFlags depthFlags);
    void bindBlendSettings(BlendFlags);

    void bindRenderTarget(TargetFlags, DepthStencilFlags, BlendFlags);
    void bindPixelShader(const std::string& ps_name);

    template <typename T>
    void bindPixelSB(const StructuredBuffer& sb, unsigned int slot)
    {
        context->getContext()->PSSetShaderResources(slot, 1, &sb.srv);
    }
    void bindPixelTexture(uint8_t slot,
                          const Texture& texture,
                          SamplerType samplerType);

    // Draw Calls. Set tri_end to -1 if you want it to draw all triangles
    // after tri_start.
    void drawMesh(const Mesh* mesh,
                  UINT instance_count,
                  int tri_start = INDEX_LIST_START,
                  int tri_end = INDEX_LIST_END);
    void drawPostProcessQuad();

    // Render to Screen
    void endFrame();

  private:
    struct Stats
    {
        uint32_t numDraws = 0;
    };
    Stats stats;
    void imGui();

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