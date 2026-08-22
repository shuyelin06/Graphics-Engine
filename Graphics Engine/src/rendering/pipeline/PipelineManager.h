#pragma once

#include <cstdint>

#include "../Direct3D11.h"
#include "rendering/core/Device.h"
#include "rendering/core/RenderSettings.h"
#include "rendering/core/Texture.h"
#include "rendering/core/VertexStreamIDs.h"

#include "StructuredBuffer.h"

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

// PipelineManager Class:
// Provides an interface for working with the 3D rendering pipeline.
// Uses D3D under the hood.
class Texture;

class Pipeline
{
  private:
    // D3D Interfaces
    HWND window;
    std::unique_ptr<Device> device;
    std::unique_ptr<DeviceContext> context;

    // Swapchain and Render Targets
    std::shared_ptr<Texture> render_target_dest = nullptr;
    std::shared_ptr<Texture> render_target_src = nullptr;
    std::shared_ptr<Texture> depth_stencil = nullptr;

    TargetFlags flag_target;
    DepthSettings flag_depth;
    BlendSettings flag_blend;

    std::shared_ptr<Geometry> postprocessQuad;

    void initializeTargets(HWND window);

  public:
    Pipeline(HWND window);
    ~Pipeline();

    Device* getDevice() const;
    DeviceContext* getContext() const;
    const std::shared_ptr<Texture> getRenderTargetDest() const;
    const std::shared_ptr<Texture> getRenderTargetSrc() const;
    const std::shared_ptr<Texture> getDepthStencil() const;

    // Prepare
    void beginFrame(const uint64_t frame);

    // Vertex Technique API
    void setVertexTopology(VertexTopology topology);

    // Pixel Technique API
    void bindRenderTarget(TargetFlags, DepthSettings, BlendSettings);

    void drawPostProcessQuad();

    // Render to Screen
    void endFrame();

    void swapActiveTarget();

  private:
    struct Stats
    {
        uint32_t numDraws = 0;
    };
    Stats stats;
    void imGui();

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