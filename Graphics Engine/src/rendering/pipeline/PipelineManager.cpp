#include "PipelineManager.h"

#include <assert.h>

#include "../core/Mesh.h"
#include "math/Vector4.h"

#if defined(_DEBUG)
#include "../ImGui.h"
#include "../util/CPUTimer.h"
#include "../util/GPUTimer.h"
#endif

namespace Engine {
using namespace Math;

namespace Graphics {
Pipeline::Pipeline(HWND window) {
    // Initialize my device, context, and render targets
    initializeTargets(window);

    // Initialize my shader manager
    shader_manager = new ShaderManager(device);
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

    // Initialize my constant buffer handles
    for (int i = 0; i < CBSlot::CBCOUNT; i++) {
        vcb_handles[i] = new CBHandle();
        pcb_handles[i] = new CBHandle();
    }

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

        device->CreateBuffer(&buffer_desc, &sr_data, &postprocess_quad);
    }

    initializeSamplers();
    bindSamplers();

#if defined(_DEBUG)
    imGuiInitialize(window);
    imGuiPrepare();
#endif
}

Pipeline::~Pipeline() {
#if defined(_DEBUG)
    imGuiShutdown();
#endif

    for (int i = 0; i < CBSlot::CBCOUNT; i++) {
        delete vcb_handles[i];
        delete pcb_handles[i];
    }

    for (int i = 0; i < SamplerSlot::SamplerCount; i++) {
        if (samplers[i] != nullptr)
            samplers[i]->Release();
    }

    delete shader_manager;
}

void Pipeline::initializeTargets(HWND _window) {
    HRESULT result;

    // Get my window width and height
    window = _window;

    RECT rect;
    GetClientRect(window, &rect);
    const UINT width = rect.right - rect.left;
    const UINT height = rect.bottom - rect.top;
    viewport = {0.0f, 0.0f, (float)width, (float)height, 0.0f, 1.0f};

    // Create my swap chain. This will let me swap between textures for
    // rendering, so the user doesn't see the next frame while it's being
    // rendered.
    {
        DXGI_SWAP_CHAIN_DESC swap_chain_descriptor = {0};

        swap_chain_descriptor.BufferDesc.RefreshRate.Numerator = 0;
        swap_chain_descriptor.BufferDesc.RefreshRate.Denominator = 1;
        swap_chain_descriptor.BufferDesc.Width = width;
        swap_chain_descriptor.BufferDesc.Height = height;
        swap_chain_descriptor.BufferDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
        swap_chain_descriptor.SampleDesc.Count = 1;
        swap_chain_descriptor.SampleDesc.Quality = 0;
        swap_chain_descriptor.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        swap_chain_descriptor.BufferCount = 1; // # Back Buffers
        swap_chain_descriptor.OutputWindow = window;
        swap_chain_descriptor.Windowed = true; // Displaying to a Window

        D3D_FEATURE_LEVEL feature_level; // Stores the GPU functionality
        result = D3D11CreateDeviceAndSwapChain(
            NULL, D3D_DRIVER_TYPE_HARDWARE, NULL,
            0, // Flags
            NULL, 0, D3D11_SDK_VERSION, &swap_chain_descriptor, &swapchain,
            &device, &feature_level, &context);
        assert(S_OK == result && swapchain && device && context);
    }

    // Create my screen target with the swap chain's frame buffer. This
    // will store my output image.
    ID3D11Texture2D* tex;

    {
        result =
            swapchain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&tex);
        assert(SUCCEEDED(result));
        screen_target = new Texture(tex, width, height);

        result = device->CreateRenderTargetView(screen_target->texture, 0,
                                                &screen_target->target_view);
        assert(SUCCEEDED(result));

        // Free frame buffer (no longer needed)
        screen_target->texture->Release();
    }

    D3D11_TEXTURE2D_DESC tex_desc = {};
    D3D11_SHADER_RESOURCE_VIEW_DESC srv_desc = {};

    // Create 2 render targets. We will ping pong between
    // these two render targets during post processing.
    {
        tex_desc.Width = width;
        tex_desc.Height = height;
        tex_desc.MipLevels = 1;
        tex_desc.ArraySize = 1;
        tex_desc.MipLevels = 1;
        tex_desc.ArraySize = 1;
        tex_desc.SampleDesc.Count = 1;
        tex_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
        tex_desc.BindFlags =
            D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;

        srv_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
        srv_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
        srv_desc.Texture2D.MostDetailedMip = 0;
        srv_desc.Texture2D.MipLevels = 1;

        render_target_src = new Texture(device, tex_desc);
        render_target_src->createShaderResourceView(device, srv_desc);
        render_target_src->createRenderTargetView(device);

        render_target_dest = new Texture(device, tex_desc);
        render_target_dest->createShaderResourceView(device, srv_desc);
        render_target_dest->createRenderTargetView(device);
    }

    // Create another texture as a depth stencil, for z-testing
    // Create my depth stencil which will be used for z-tests
    // 24 Bits for Depth, 8 Bits for Stencil
    {
        tex_desc.Width = width;
        tex_desc.Height = height;
        tex_desc.MipLevels = 1;
        tex_desc.ArraySize = 1;
        tex_desc.MipLevels = 1;
        tex_desc.ArraySize = 1;
        tex_desc.SampleDesc.Count = 1;
        tex_desc.Format = DXGI_FORMAT_R24G8_TYPELESS;
        tex_desc.BindFlags =
            D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;

        srv_desc.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
        srv_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
        srv_desc.Texture2D.MostDetailedMip = 0;
        srv_desc.Texture2D.MipLevels = 1;

        D3D11_DEPTH_STENCIL_VIEW_DESC desc_stencil = {};
        desc_stencil.Format =
            DXGI_FORMAT_D24_UNORM_S8_UINT; // Same format as texture
        desc_stencil.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DMS;

        depth_stencil = new Texture(device, tex_desc);
        depth_stencil->createDepthStencilView(device, desc_stencil);
        depth_stencil->createShaderResourceView(device, srv_desc);

        depth_stencil_copy = new Texture(device, tex_desc);
        // TEMP
        depth_stencil_copy->createDepthStencilView(device, desc_stencil);
        depth_stencil_copy->createShaderResourceView(device, srv_desc);
    }

    // Create my depth states
    {
        D3D11_DEPTH_STENCIL_DESC desc = {};

        depth_states[Depth_Disabled] = nullptr;

        // Enable depth testing
        desc.DepthEnable = TRUE;
        // Standard depth test
        desc.DepthFunc = D3D11_COMPARISON_LESS;
        // Enable depth writing
        desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
        // No stencil testing
        desc.StencilEnable = FALSE;

        result = device->CreateDepthStencilState(
            &desc, &depth_states[Depth_TestAndWrite]);
        assert(SUCCEEDED(result));

        // Enable depth testing
        desc.DepthEnable = TRUE;
        // Standard depth test
        desc.DepthFunc = D3D11_COMPARISON_LESS;
        // Disable depth writing
        desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
        // No stencil testing
        desc.StencilEnable = FALSE;

        HRESULT result = device->CreateDepthStencilState(
            &desc, &depth_states[Depth_TestNoWrite]);
        assert(SUCCEEDED(result));
    }

    // Create my blend states
    {
        D3D11_BLEND_DESC blend_desc = {};

        blend_desc.RenderTarget[0].BlendEnable = TRUE;
        blend_desc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
        blend_desc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
        blend_desc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
        blend_desc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_SRC_ALPHA;
        blend_desc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA;
        blend_desc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_MAX;
        blend_desc.RenderTarget[0].RenderTargetWriteMask =
            D3D11_COLOR_WRITE_ENABLE_ALL;
        result = device->CreateBlendState(&blend_desc,
                                          &blend_states[Blend_SrcAlphaOnly]);
        assert(SUCCEEDED(result));

        blend_desc.RenderTarget[0].BlendEnable = TRUE;
        blend_desc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
        blend_desc.RenderTarget[0].DestBlend = D3D11_BLEND_DEST_ALPHA;
        blend_desc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
        blend_desc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_SRC_ALPHA;
        blend_desc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_DEST_ALPHA;
        blend_desc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_MAX;
        blend_desc.RenderTarget[0].RenderTargetWriteMask =
            D3D11_COLOR_WRITE_ENABLE_ALL;
        result = device->CreateBlendState(&blend_desc,
                                          &blend_states[Blend_UseSrcAndDest]);
        assert(SUCCEEDED(result));
    }
}

// InitializeSamplers:
// Initializes the most commonly used samplers in the pipeline.
// These samplers will not be rebound over the entire program.
void Pipeline::initializeSamplers() {
    D3D11_SAMPLER_DESC sampler_desc = {};
    ID3D11SamplerState* sampler = NULL;

    for (int i = 0; i < SamplerCount; i++)
        samplers[i] = NULL;

    // Point Sampler: Index 0
    sampler_desc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
    sampler_desc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
    sampler_desc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
    sampler_desc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;

    device->CreateSamplerState(&sampler_desc, &sampler);
    assert(sampler != NULL);

    samplers[Point] = sampler;

    // Shadow Sampler: Index 1
    sampler_desc.Filter =
        D3D11_FILTER_MIN_MAG_MIP_LINEAR; // Linear Filtering for PCF
    sampler_desc.AddressU = D3D11_TEXTURE_ADDRESS_BORDER;
    sampler_desc.AddressV = D3D11_TEXTURE_ADDRESS_BORDER;
    sampler_desc.AddressW = D3D11_TEXTURE_ADDRESS_BORDER;
    sampler_desc.BorderColor[0] = 0.f;
    sampler_desc.BorderColor[1] = 0.f;
    sampler_desc.BorderColor[2] = 0.f;
    sampler_desc.BorderColor[3] = 0.f;
    sampler_desc.ComparisonFunc = D3D11_COMPARISON_NEVER;
    sampler_desc.MinLOD = 0;
    sampler_desc.MaxLOD = 1.0f;

    device->CreateSamplerState(&sampler_desc, &sampler);
    assert(sampler != NULL);

    samplers[Shadow] = sampler;
}

ID3D11Device* Pipeline::getDevice() const { return device; }
ID3D11DeviceContext* Pipeline::getContext() const { return context; }
Texture* Pipeline::getRenderTargetDest() const { return render_target_dest; }
Texture* Pipeline::getRenderTargetSrc() const { return render_target_src; }
Texture* Pipeline::getDepthStencil() const { return depth_stencil; }
Texture* Pipeline::getDepthStencilCopy() const { return depth_stencil_copy; }

// Prepare
void Pipeline::prepare() {
    // Clear the the target destination color
    render_target_dest->clearAsRenderTarget(context, Color(0.f, 0.f, 0.f));
}

// Shader Management
bool Pipeline::bindVertexShader(const std::string& vs_name) {
    VertexShader* new_shader = shader_manager->getVertexShader(vs_name);

    // Check if shader exists
    if (new_shader == nullptr)
        assert(false);

    // If the new vertex shader has a different layout, invalidate
    // the active mesh pool as we'll have to upload additional stream data
    if (active_pool_addr != NULL && vs_active->layout != new_shader->layout)
        active_pool_addr = NULL;

    // Save the active vertex shader
    vs_active = new_shader;

    // Bind shader and input layout
    context->IASetInputLayout(vs_active->layout);
    context->VSSetShader(vs_active->shader, NULL, 0);

    return true;
}

bool Pipeline::bindPixelShader(const std::string& ps_name) {
    ps_active = shader_manager->getPixelShader(ps_name);

    // Check if shader exists
    if (ps_active == nullptr)
        assert(false);

    // Bind shader
    context->PSSetShader(ps_active->shader, NULL, 0);

    return true;
}

// Render Target Binding
void Pipeline::bindRenderTarget(TargetFlags f_target, DepthStencilFlags f_depth,
                                BlendFlags f_blend) {
    flag_target = f_target;
    flag_depth = f_depth;
    flag_blend = f_blend;

    // Handle render target flags
    ID3D11RenderTargetView* target_view = nullptr;

    switch (f_target) {
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

    if (f_depth != Depth_Disabled) {
        depth_view = depth_stencil->depth_view;
        ID3D11DepthStencilState* state = depth_states[f_depth];
        context->OMSetDepthStencilState(state, 0);
    }

    context->OMSetRenderTargets(1, &target_view, depth_view);
    context->RSSetViewports(1, &viewport);

    // Handle blend flags
    context->OMSetBlendState(blend_states[f_blend], nullptr, 0xFFFFFFFF);
}

void Pipeline::swapActiveTarget() {
    Texture* temp = render_target_dest;
    render_target_dest = render_target_src;
    render_target_src = temp;
}

void Pipeline::bindInactiveTarget(int slot) {
    context->PSSetShaderResources(slot, 1, &render_target_src->shader_view);
}
void Pipeline::bindDepthStencil(int slot) {
    assert(flag_depth == Depth_Disabled || flag_depth == Depth_TestNoWrite);
    context->PSSetShaderResources(slot, 1, &depth_stencil->shader_view);
}

void Pipeline::bindSamplers() {
    context->PSSetSamplers(0, SamplerCount, samplers);
}

// Constant Buffer Loading
IConstantBuffer Pipeline::loadVertexCB(CBSlot slot) {
    return IConstantBuffer(vcb_handles[slot], slot, CBVertex, device, context);
}
IConstantBuffer Pipeline::loadPixelCB(CBSlot slot) {
    return IConstantBuffer(pcb_handles[slot], slot, CBPixel, device, context);
}

void Pipeline::drawMesh(const Mesh* mesh, int tri_start, int tri_end,
                        UINT instance_count) {
    // This is failing?
    // assert((vs_active->layout_pin & mesh->layout) == vs_active->layout_pin);
    const MeshPool* pool = mesh->buffer_pool;

    // All meshes are assumed to havae a triangle list topology.
    // While there are more efficient representations, this is done
    // for simplicity.
    context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    // UNTESTED: If the MeshPool is the same, we don't need to rebind anything
    // if (pool != active_pool_addr) {
    active_pool_addr = pool;

    // Bind my index buffer. All meshes are assumed to have one index
    // buffer, associated with multiple vertex buffers.
    context->IASetIndexBuffer(pool->ibuffer, DXGI_FORMAT_R32_UINT, 0);

    // Iterate through the layout of my vertex shader
    // and bind the vertex buffers that the vertex shader needs.
    memset(vb_buffers, 0, sizeof(ID3D11Buffer*) * BINDABLE_STREAM_COUNT);
    for (int i = 0; i < BINDABLE_STREAM_COUNT; i++) {
        if (((1 << i) & vs_active->layout_pin) == (1 << i)) {
            vb_buffers[i] = pool->vbuffers[i];
        }
    }

    context->IASetVertexBuffers(0, BINDABLE_STREAM_COUNT, vb_buffers,
                                vb_strides, vb_offsets);
    //}

    // Issue my draw call. We will always draw indexed instanced, even if the
    // number of instances is 1.
    const UINT index_start = (mesh->triangle_start + tri_start) * 3;
    const UINT num_indices =
        (tri_end == -1) ? mesh->num_triangles * 3 : (tri_end - tri_start) * 3;
    const UINT index_offset = mesh->vertex_start;

    context->DrawIndexedInstanced(num_indices, instance_count, index_start,
                                  index_offset, 0);
}

void Pipeline::drawPostProcessQuad() {
    const UINT vertexStride = sizeof(float) * 4;
    const UINT vertexOffset = 0;

    context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    context->IASetVertexBuffers(0, 1, &postprocess_quad, &vertexStride,
                                &vertexOffset);

    context->Draw(6, 0);
}

// Present:
// Display everything we've rendered onto the screen
void Pipeline::present() {
    // Execute a shader to transfer the pixel data from our
    // current dest render target to the screen target.
    {
#if defined(_DEBUG)
        IGPUTimer gpu_timer = GPUTimer::TrackGPUTime("Render Finish Pass");
#endif

        bindVertexShader("PostProcess");
        bindPixelShader("PostProcess");

        context->OMSetRenderTargets(1, &screen_target->target_view, nullptr);
        context->RSSetViewports(1, &viewport);
        context->OMSetBlendState(blend_states[Blend_SrcAlphaOnly], nullptr,
                                 0xFFFFFFFF);

        context->PSSetShaderResources(0, 1, &render_target_dest->shader_view);

        drawPostProcessQuad();
    }

#if defined(_DEBUG)
    // Begin timestamping
    GPUTimer::EndFrame();
    imGuiFinish();
#endif

    swapchain->Present(1, 0);

#if defined(_DEBUG)
    imGuiPrepare();
#endif
}

#if defined(_DEBUG) // ImGui
// ImGui Initialize:
// Initializes the ImGui menu and associated data.
void Pipeline::imGuiInitialize(HWND window) {
    // Initialize ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |=
        ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls
    io.ConfigFlags |=
        ImGuiConfigFlags_NavEnableGamepad; // Enable Gamepad Controls

    ImGui_ImplWin32_Init(window);
    ImGui_ImplDX11_Init(device, context);

    // Create GPU + CPU Timers
    GPUTimer::Initialize(device, context);
    CPUTimer::Initialize();
}

// ImGuiPrepare:
// Creates a new frame for the ImGui system and begin tracking GPU time
// for the current frame
void Pipeline::imGuiPrepare() {
    // Start the Dear ImGui frame
    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();
    ImGui::BeginMainMenuBar();
}

// ImGuiFinish:
// Finish and present the ImGui window
void Pipeline::imGuiFinish() {
    if (ImGui::BeginMenu("CPU / GPU Runtime")) {
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
void Pipeline::imGuiShutdown() {
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
}
#endif

} // namespace Graphics
} // namespace Engine