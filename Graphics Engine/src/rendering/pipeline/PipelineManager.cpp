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

    // Initialize my shader manager
    shader_manager = new ShaderManager(device->getDevice());
    shader_manager->initializeShaders();

    // Initialize my vertex buffers / offsets / strides
    active_pool_addr = NULL;
    memset(vb_buffers, 0, sizeof(ID3D11Buffer*) * BINDABLE_STREAM_COUNT);
    memset(vb_strides, 0, sizeof(UINT) * BINDABLE_STREAM_COUNT);
    memset(vb_offsets, 0, sizeof(UINT) * BINDABLE_STREAM_COUNT);

    vb_strides[POSITION] = sizeof(float) * 3;
    vb_strides[TEXTURE] = sizeof(float) * 2;
    vb_strides[NORMAL] = sizeof(float) * 3;
    vb_strides[COLOR] = sizeof(float) * 3;
    vb_strides[JOINTS] = sizeof(float) * 4;
    vb_strides[WEIGHTS] = sizeof(float) * 4;

    // Initialize my full screen quad
    {
        const Vector4 fullscreen_quad[6] = {
            // First Triangle
            Vector4(-1, -1, 0, 1), Vector4(-1, 1, 0, 1), Vector4(1, 1, 0, 1),
            // Second Triangle
            Vector4(-1, -1, 0, 1), Vector4(1, 1, 0, 1), Vector4(1, -1, 0, 1)};

        D3D11_BUFFER_DESC buffer_desc = {};
        buffer_desc.ByteWidth = sizeof(fullscreen_quad);
        buffer_desc.Usage = D3D11_USAGE_DEFAULT;
        buffer_desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;

        D3D11_SUBRESOURCE_DATA sr_data = {};
        sr_data.pSysMem = (void*)fullscreen_quad;

        device->getDevice()->CreateBuffer(&buffer_desc, &sr_data,
                                          &postprocess_quad);
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

    delete shader_manager;
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

void Pipeline::setVertexTopology(VertexTopology topology)
{
    switch (topology)
    {
    case VertexTopology::TriangleList:
        context->getContext()->IASetPrimitiveTopology(
            D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        break;

    case VertexTopology::LineList:
        context->getContext()->IASetPrimitiveTopology(
            D3D11_PRIMITIVE_TOPOLOGY_LINELIST);
        break;

    default:
        assert(false);
    }
}

// Shader Management
void Pipeline::bindVertexShader(const std::string& vs_name)
{
    VertexShader* newVS = shader_manager->getVertexShader(vs_name);
    assert(newVS);

    if (newVS != vs_active)
    {
        vs_active = newVS;

        // Bind shader and input layout
        context->getContext()->IASetInputLayout(vs_active->layout);
        context->getContext()->VSSetShader(vs_active->shader, NULL, 0);
    }
}

void Pipeline::bindPixelShader(const std::string& ps_name)
{
    PixelShader* newPS = shader_manager->getPixelShader(ps_name);
    assert(newPS);

    if (newPS != ps_active)
    {
        ps_active = newPS;
        context->getContext()->PSSetShader(ps_active->shader, NULL, 0);
    }
}

void Pipeline::bindRenderTarget(TargetFlags f_target,
                                DepthStencilFlags f_depth,
                                BlendFlags f_blend)
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

void Pipeline::drawMesh(const Mesh* mesh,
                        UINT instance_count,
                        int tri_start,
                        int tri_end)
{
    assert(mesh);
    assert(mesh->buffer_pool);

    const MeshPool* pool = mesh->buffer_pool;
    assert(pool->layout.vertexLayoutSupports(vs_active->vertexLayout));

    // All meshes are assumed to havae a triangle list topology.
    // While there are more efficient representations, this is done
    // for simplicity.
    context->getContext()->IASetPrimitiveTopology(
        D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    if (pool != active_pool_addr)
    {
        active_pool_addr = pool;

        // Bind my index buffer. All meshes are assumed to have one index
        // buffer, associated with multiple vertex buffers.
        context->getContext()->IASetIndexBuffer(pool->ibuffer,
                                                DXGI_FORMAT_R32_UINT, 0);

        // Iterate through the layout of my pool and bind all vertex buffers.
        memset(vb_buffers, 0, sizeof(ID3D11Buffer*) * BINDABLE_STREAM_COUNT);
        for (int i = 0; i < BINDABLE_STREAM_COUNT; i++)
        {
            if (pool->layout.hasVertexStream((VertexDataStream)i))
            {
                vb_buffers[i] = pool->vbuffers[i];
            }
        }

        context->getContext()->IASetVertexBuffers(
            0, BINDABLE_STREAM_COUNT, vb_buffers, vb_strides, vb_offsets);
    }

    // Issue my draw call. We will always draw indexed instanced, even if the
    // number of instances is 1.
    const UINT index_start = (mesh->triangle_start + tri_start) * 3;
    const UINT num_indices =
        (tri_end == -1) ? mesh->num_triangles * 3 : (tri_end - tri_start) * 3;
    const UINT index_offset = mesh->vertex_start;

    context->getContext()->DrawIndexedInstanced(num_indices, instance_count,
                                                index_start, index_offset, 0);

    stats.numDraws++;
}

void Pipeline::drawPostProcessQuad()
{
    const UINT vertexStride = sizeof(float) * 4;
    const UINT vertexOffset = 0;

    context->getContext()->IASetPrimitiveTopology(
        D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    context->getContext()->IASetVertexBuffers(0, 1, &postprocess_quad,
                                              &vertexStride, &vertexOffset);

    context->getContext()->Draw(6, 0);
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

        bindVertexShader("PostProcess");
        bindPixelShader("PostProcess");

        context->bindRenderTarget(nullptr, nullptr,
                                  DepthStencilFlags::Depth_Disabled,
                                  BlendFlags::SrcAlphaOnly);
        context->bindPixelTexture(0, render_target_dest,
                                  SamplerType::Sampler_Point);

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