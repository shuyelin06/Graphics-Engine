#include "PipelineManager.h"

#include <assert.h>

#include "../core/Mesh.h"
#include "math/Vector4.h"

#if defined(_DEBUG)
#include "../ImGui.h"
#include "../util/CPUTimer.h"
#include "../util/GPUTimer.h"
#endif

namespace Engine
{
using namespace Math;

namespace Graphics
{
Pipeline::Pipeline(HWND window)
{
    // Initialize my device, context->getContext(), and render targets
    initializeTargets(window);

    // Initialize my full screen quad
    {
        const Vector4 fullscreen_quad[6] = {
            // First Triangle
            Vector4(-1, -1, 0, 1), Vector4(-1, 1, 0, 1), Vector4(1, 1, 0, 1),
            // Second Triangle
            Vector4(-1, -1, 0, 1), Vector4(1, 1, 0, 1), Vector4(1, -1, 0, 1)};

        std::shared_ptr<Buffer> fullScreenQuad = device->createBuffer(
            "Full Screen Quad", BufferType::Vertex, sizeof(fullscreen_quad),
            fullscreen_quad, false);

        postprocessQuad = std::make_shared<Geometry>();
        postprocessQuad->vertexBuffers[VertexDataStream::PosXYZ_TexU] =
            fullScreenQuad;
        postprocessQuad->indexCount = 6;
    }

#if defined(_DEBUG)
    imGuiInitialize(window);
    imGuiPrepare();
#endif

    ImGuiHelper::registerImGuiCallback("Render/Pipeline",
                                       [this]() { imGui(); });
}

Pipeline::~Pipeline()
{
#if defined(_DEBUG)
    imGuiShutdown();
#endif
}

void Pipeline::initializeTargets(HWND _window)
{
    HRESULT result;

    // Get my window width and height
    window = _window;

    RECT rect;
    GetClientRect(window, &rect);
    const UINT width = rect.right - rect.left;
    const UINT height = rect.bottom - rect.top;

    // Create my swap chain. This will let me swap between textures for
    // rendering, so the user doesn't see the next frame while it's being
    // rendered.
    InitializeGraphicsAPI(window, device, context);

    render_target_src = device->createTexture(
        "Render Target Source", TextureLayout::R8G8B8A8_UNORM_SGRB,
        TextureUsage::RenderTarget | TextureUsage::ShaderResource, width,
        height);
    render_target_dest = device->createTexture(
        "Render Target Destination", TextureLayout::R8G8B8A8_UNORM_SGRB,
        TextureUsage::RenderTarget | TextureUsage::ShaderResource, width,
        height);

    depth_stencil = device->createTexture(
        "Depth Stencil", TextureLayout::R24_UNORM_G8_UINT,
        TextureUsage::DepthStencil | TextureUsage::ShaderResource, width,
        height);
}

Device* Pipeline::getDevice() const { return device.get(); }
DeviceContext* Pipeline::getContext() const { return context.get(); }
const std::shared_ptr<Texture> Pipeline::getRenderTargetDest() const
{
    return render_target_dest;
}
const std::shared_ptr<Texture> Pipeline::getRenderTargetSrc() const
{
    return render_target_src;
}
const std::shared_ptr<Texture> Pipeline::getDepthStencil() const
{
    return depth_stencil;
}

// Prepare
void Pipeline::beginFrame(const uint64_t frame)
{
    // Clear the the target destination color
    GPUTimer::BeginFrame(frame);
    stats = Pipeline::Stats();

    const float baseColor[4] = {0.f, 0.f, 0.f, 1.f};
    context->clearRenderTarget(render_target_dest, baseColor);
}

void Pipeline::bindRenderTarget(TargetFlags f_target,
                                DepthSettings f_depth,
                                BlendSettings f_blend)
{
    /*
    flag_target = f_target;
    flag_depth = f_depth;
    flag_blend = f_blend;

    // Handle render target flags
    ID3D11RenderTargetView* target_view = nullptr;

    switch (f_target)
    {
    case Target_SwapTarget:
        swapActiveTarget();
        [[fallthrough]];
    case Target_UseExisting:
        target_view = render_target_dest->target_view;
        break;

    default:
        break;
    }

    // Handle depth stencil flags
    ID3D11DepthStencilView* depth_view = nullptr;

    if (f_depth != Depth_Disabled)
    {
        depth_view = depth_stencil->depth_view;
        ID3D11DepthStencilState* state = depth_states[f_depth];
        context->getContext()->OMSetDepthStencilState(state, 0);
    }

    context->getContext()->OMSetRenderTargets(1, &target_view, depth_view);
    context->getContext()->RSSetViewports(1, &viewport);

    // Handle blend flags
    context->getContext()->OMSetBlendState(blend_states[f_blend], nullptr,
                                           0xFFFFFFFF);
                                           */
}

void Pipeline::swapActiveTarget()
{
    std::swap(render_target_src, render_target_dest);
}

void Pipeline::drawPostProcessQuad()
{
    context->getContext()->IASetPrimitiveTopology(
        D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    context->draw(postprocessQuad.get(), 1);
}

// Present:
// Display everything we've rendered onto the screen
void Pipeline::endFrame()
{
    // Execute a shader to transfer the pixel data from our
    // current dest render target to the screen target.
    {
#if defined(_DEBUG)
        IGPUTimer gpu_timer = GPUTimer::TrackGPUTime("Render Finish Pass");
#endif

        context->bindShaderProgram("PostProcess", "PostProcess");

        context->bindRenderTarget(nullptr, nullptr,
                                  DepthSettings::Depth_Disabled,
                                  BlendSettings::SrcAlphaOnly);
        context->bindPixelTexture(0, render_target_dest,
                                  SamplerSettings::Point);

        drawPostProcessQuad();
    }

#if defined(_DEBUG)
    GPUTimer::EndFrame();
    imGuiFinish();
#endif

    context->present();

#if defined(_DEBUG)
    imGuiPrepare();
#endif
}

#if defined(_DEBUG) // ImGui
// ImGui Initialize:
// Initializes the ImGui menu and associated data.
void Pipeline::imGuiInitialize(HWND window)
{
    // Initialize ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |=
        ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls
    io.ConfigFlags |=
        ImGuiConfigFlags_NavEnableGamepad; // Enable Gamepad Controls

    ImGui_ImplWin32_Init(window);
    ImGui_ImplDX11_Init(device->getDevice(), context->getContext());

    // Create GPU + CPU Timers
    GPUTimer::Initialize(device->getDevice(), context->getContext());
    CPUTimer::Initialize();
}

// ImGuiPrepare:
// Creates a new frame for the ImGui system and begin tracking GPU time
// for the current frame
void Pipeline::imGuiPrepare()
{
    // Start the Dear ImGui frame
    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();
    ImGui::BeginMainMenuBar();
}

// ImGuiFinish:
// Finish and present the ImGui window
void Pipeline::imGuiFinish()
{
    if (ImGui::BeginMenu("CPU / GPU Runtime"))
    {
        ImGui::SeparatorText("CPU Times:");
        CPUTimer::DisplayCPUTimes();

        ImGui::SeparatorText("GPU Times:");
        GPUTimer::DisplayGPUTimes();

        ImGui::EndMenu();
    }

    ImGui::EndMainMenuBar();

    // Finish the ImGui Frame
    ImGui::Render();
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
}

// ImGuiShutDown:
// Shut down the ImGui system
void Pipeline::imGuiShutdown()
{
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
}
#endif

void Pipeline::imGui()
{
#if defined(IMGUI_ENABLED)
    ImGui::Text("Draw Call Count: %zu", stats.numDraws);
#endif
}

} // namespace Graphics
} // namespace Engine