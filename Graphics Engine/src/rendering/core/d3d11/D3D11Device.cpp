#include "D3D11Device.h"

#include <assert.h>

#include "D3D11Buffer.h"
#include "D3D11Shader.h"
#include "D3D11Texture.h"

namespace Engine
{
namespace Graphics
{
void InitializeGraphicsAPI(HWND window,
                           std::unique_ptr<Device>& outDevice,
                           std::unique_ptr<DeviceContext>& outContext)
{
    ID3D11Device* device = nullptr;
    ID3D11DeviceContext* context = nullptr;
    IDXGISwapChain* swapchain = nullptr;
    ID3D11RenderTargetView* target = nullptr;

    RECT rect;
    GetClientRect(window, &rect);
    const UINT width = rect.right - rect.left;
    const UINT height = rect.bottom - rect.top;
    D3D11_VIEWPORT viewport = {0.0f,          0.0f, (float)width,
                               (float)height, 0.0f, 1.0f};

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
    HRESULT result = D3D11CreateDeviceAndSwapChain(
        NULL, D3D_DRIVER_TYPE_HARDWARE, NULL,
        D3D11_CREATE_DEVICE_DEBUG, // Flags
        NULL, 0, D3D11_SDK_VERSION, &swap_chain_descriptor, &swapchain, &device,
        &feature_level, &context);
    assert(S_OK == result && device && swapchain && context);

    // Create my screen target with the swap chain's frame buffer. This
    // will store my output image.
    ID3D11Texture2D* texture;
    result =
        swapchain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&texture);
    assert(SUCCEEDED(result));

    result = device->CreateRenderTargetView(texture, 0, &target);
    assert(SUCCEEDED(result));

    // Free frame buffer (no longer needed)
    texture->Release();

    std::unique_ptr<Direct3D11Device> d3dDevice =
        std::make_unique<Direct3D11Device>(device);
    outContext = std::make_unique<Direct3D11DeviceContext>(
        context, device, swapchain, target, viewport, d3dDevice->getShaders());
    outDevice = std::move(d3dDevice);
}

Direct3D11Device::Direct3D11Device(ID3D11Device* device)
    : device(device)
{
    shaders = std::make_unique<D3D11ShaderCompiler>(device);
    shaders->initializeShaders();
}
Direct3D11Device::~Direct3D11Device() { device->Release(); }

ID3D11Device* Direct3D11Device::getDevice() { return device; }

void Direct3D11Device::reloadShaders() { shaders->initializeShaders(); }

std::shared_ptr<Buffer> Direct3D11Device::createBuffer(const char* debugName,
                                                       BufferType type,
                                                       size_t byteSize,
                                                       const void* initData,
                                                       bool dynamic)
{
    return std::make_shared<D3D11Buffer>(device, type, byteSize, initData,
                                         dynamic);
}

std::shared_ptr<Texture> Direct3D11Device::createTexture(const char* debugName,
                                                         TextureLayout layout,
                                                         TextureUsage usage,
                                                         uint32_t width,
                                                         uint32_t height,
                                                         uint16_t slices,
                                                         uint8_t mips,
                                                         bool dynamic,
                                                         const void* src)
{
    return std::make_shared<D3D11Texture>(device, layout, usage, width, height,
                                          slices, mips, dynamic, src);
}

Direct3D11DeviceContext::Direct3D11DeviceContext(ID3D11DeviceContext* context,
                                                 ID3D11Device* device,
                                                 IDXGISwapChain* swapchain,
                                                 ID3D11RenderTargetView* target,
                                                 D3D11_VIEWPORT viewport,
                                                 D3D11ShaderCompiler* shaders)
    : context(context)
    , device(device)
    , swapchain(swapchain)
    , screenTarget(target)
    , screenViewport(viewport)
    , shaders(shaders)
{
    memset(vb_buffers, 0, sizeof(ID3D11Buffer*) * BINDABLE_STREAM_COUNT);
    memset(vb_strides, 0, sizeof(UINT) * BINDABLE_STREAM_COUNT);
    memset(vb_offsets, 0, sizeof(UINT) * BINDABLE_STREAM_COUNT);

    for (int i = 0; i < BINDABLE_STREAM_COUNT; i++)
    {
        vb_strides[i] = VertexLayout::VertexStreamStride((VertexDataStream)i);
    }

    initializeDepthStates();
    initializeBlendStates();
    initializeSamplers();
}
Direct3D11DeviceContext ::~Direct3D11DeviceContext()
{
    context->Release();
    swapchain->Release();
    screenTarget->Release();

    for (uint8_t slot = 0; slot < kVertexConstantBufferMax; slot++)
    {
        if (vertexCB[slot])
            vertexCB[slot]->Release();
    }
    for (uint8_t slot = 0; slot < kPixelConstantBufferMax; slot++)
    {
        if (pixelCB[slot])
            pixelCB[slot]->Release();
    }
}

void Direct3D11DeviceContext::initializeDepthStates()
{
    D3D11_DEPTH_STENCIL_DESC desc = {};
    HRESULT result;

    depth_states[(int)DepthSettings::Depth_Disabled] = nullptr;

    // Enable depth testing
    desc.DepthEnable = TRUE;
    // Standard depth test
    desc.DepthFunc = D3D11_COMPARISON_LESS;
    // Enable depth writing
    desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
    // No stencil testing
    desc.StencilEnable = FALSE;

    result = device->CreateDepthStencilState(
        &desc, &depth_states[(int)DepthSettings::Depth_TestAndWrite]);
    assert(SUCCEEDED(result));

    // Enable depth testing
    desc.DepthEnable = TRUE;
    // Standard depth test
    desc.DepthFunc = D3D11_COMPARISON_LESS;
    // Disable depth writing
    desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
    // No stencil testing
    desc.StencilEnable = FALSE;

    result = device->CreateDepthStencilState(
        &desc, &depth_states[(int)DepthSettings::Depth_TestNoWrite]);
    assert(SUCCEEDED(result));
}

void Direct3D11DeviceContext::initializeBlendStates()
{
    D3D11_BLEND_DESC blend_desc = {};

    HRESULT result;

    blend_desc.RenderTarget[0].BlendEnable = TRUE;
    blend_desc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
    blend_desc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
    blend_desc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
    blend_desc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_SRC_ALPHA;
    blend_desc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA;
    blend_desc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_MAX;
    blend_desc.RenderTarget[0].RenderTargetWriteMask =
        D3D11_COLOR_WRITE_ENABLE_ALL;
    result = device->CreateBlendState(
        &blend_desc, &blend_states[(int)BlendSettings::SrcAlphaOnly]);
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
    result = device->CreateBlendState(
        &blend_desc, &blend_states[(int)BlendSettings::Blend_UseSrcAndDest]);
    assert(SUCCEEDED(result));
}

void Direct3D11DeviceContext::initializeSamplers()
{
    D3D11_SAMPLER_DESC sampler_desc = {};
    HRESULT result;
    // Point Sampler: Index 0
    sampler_desc = {};
    sampler_desc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
    sampler_desc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
    sampler_desc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
    sampler_desc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;

    result = device->CreateSamplerState(&sampler_desc,
                                        &samplers[(int)SamplerSettings::Point]);
    assert(SUCCEEDED(result));

    // Shadow Sampler: Index 1
    sampler_desc = {};
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

    result = device->CreateSamplerState(
        &sampler_desc, &samplers[(int)SamplerSettings::Shadow]);
    assert(SUCCEEDED(result));

    // Bilinear Sampler: Index 2
    sampler_desc = {};
    sampler_desc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    sampler_desc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
    sampler_desc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
    sampler_desc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
    sampler_desc.MinLOD = 0.0f; // holy moly!!!! this was clamping it wtf
    sampler_desc.MaxLOD = D3D11_FLOAT32_MAX;

    device->CreateSamplerState(&sampler_desc,
                               &samplers[(int)SamplerSettings::Linear]);
    assert(SUCCEEDED(result));
}

// Temporary and should be removed
ID3D11DeviceContext* Direct3D11DeviceContext::getContext() { return context; }

// Resources
void Direct3D11DeviceContext::updateBuffer(
    const std::shared_ptr<Buffer>& buffer, const void* src, size_t bytes)
{
    D3D11Buffer* buf = reinterpret_cast<D3D11Buffer*>(buffer.get());
    assert(buf);
    buf->upload(context, src, bytes);
}

void Direct3D11DeviceContext::updateTexture(
    const std::shared_ptr<Texture>& texture,
    uint8_t slice,
    const void* initData,
    size_t bytes)
{
    D3D11Texture* tex = reinterpret_cast<D3D11Texture*>(texture.get());
    assert(tex);
    tex->update(context, slice, initData, bytes);
}

void Direct3D11DeviceContext::generateMips(
    const std::shared_ptr<Texture>& texture)
{
    D3D11Texture* tex = reinterpret_cast<D3D11Texture*>(texture.get());
    assert(tex->getSRV());
    context->GenerateMips(tex->getSRV());
}

void Direct3D11DeviceContext::clearRenderTarget(
    const std::shared_ptr<Texture>& texture, const float rgba[4])
{
    D3D11Texture* tex = reinterpret_cast<D3D11Texture*>(texture.get());
    assert(tex->getTargetView());
    context->ClearRenderTargetView(tex->getTargetView(), rgba);
}

void Direct3D11DeviceContext::clearDepthStencil(
    const std::shared_ptr<Texture>& texture)
{
    D3D11Texture* tex = reinterpret_cast<D3D11Texture*>(texture.get());
    assert(tex->getDepthView());
    context->ClearDepthStencilView(
        tex->getDepthView(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.f, 0);
}

void Direct3D11DeviceContext::bindRenderTarget(
    const std::shared_ptr<Texture>& target,
    const std::shared_ptr<Texture>& depth,
    DepthSettings flags,
    BlendSettings blendFlags)
{
    D3D11Texture* targetTex = reinterpret_cast<D3D11Texture*>(target.get());
    D3D11Texture* depthTex = reinterpret_cast<D3D11Texture*>(depth.get());

    ID3D11RenderTargetView* targetView = nullptr;
    ID3D11DepthStencilView* depthView = nullptr;

    D3D11_VIEWPORT viewport = {0.f, 0.f, -1.f, -1.f, 0.f, 1.f};

    if (targetTex)
    {
        assert(targetTex->getTargetView());
        targetView = targetTex->getTargetView();

        viewport.Width = targetTex->getWidth();
        viewport.Height = targetTex->getHeight();
    }
    else
    {
        targetView = screenTarget;
        viewport = screenViewport;
    }

    if (depthTex)
    {
        assert(depthTex->getDepthView());
        ID3D11DepthStencilState* state = depth_states[(int)flags];
        context->OMSetDepthStencilState(state, 0);
        depthView = depthTex->getDepthView();

        assert(viewport.Width == -1.f ||
               viewport.Width == depthTex->getWidth());
        assert(viewport.Height == -1.f ||
               viewport.Height == depthTex->getHeight());

        if (viewport.Width == -1)
        {
            viewport.Width = depthTex->getWidth();
        }
        if (viewport.Height == -1)
        {
            viewport.Height = depthTex->getHeight();
        }
    }

    assert(targetView || depthView);

    context->OMSetRenderTargets(1, &targetView, depthView);
    context->RSSetViewports(1, &viewport);

    context->OMSetBlendState(blend_states[(int)blendFlags], nullptr,
                             0xFFFFFFFF);
}

void Direct3D11DeviceContext::bindShaderProgram(const char* vs, const char* ps)
{
    D3D11VertexShader* vsShader = shaders->getVertexShader(vs);
    D3D11PixelShader* psShader = shaders->getPixelShader(ps);

    if (vsActive != vsShader)
    {
        context->IASetInputLayout(vsShader->layout);
        context->VSSetShader(vsShader->shader, NULL, 0);
        vsActive = vsShader;
    }

    if (psActive != psShader)
    {
        context->PSSetShader(psShader->shader, NULL, 0);
        psActive = psShader;
    }

    assert(vsActive && psActive);
}

void Direct3D11DeviceContext::loadConstantBuffer(ID3D11Buffer** bufferPtr,
                                                 const void* data,
                                                 size_t bytes)
{
    HRESULT result;

    ID3D11Buffer* buffer = *bufferPtr;

    // If the buffer resource has never been created before, or the current
    // resource is too small for mapping / unmapping, create a new one.
    if (buffer == nullptr)
    {
        // Create buffer to allow dynamic usage, i.e. accessible by
        // GPU read and CPU write. We opt for this usage so that we can update
        // the resource on the fly when needed.
        D3D11_BUFFER_DESC buff_desc = {};
        buff_desc.ByteWidth = kConstantBufferMaxSize;
        buff_desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        buff_desc.Usage = D3D11_USAGE_DYNAMIC;
        buff_desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

        // Create buffer
        result = device->CreateBuffer(&buff_desc, NULL, &buffer);
        assert(SUCCEEDED(result));
    }

    assert(buffer != nullptr);

    // Perform resource renaming to update buffer data
    // Disable GPU access to data and obtain the my constant buffer resource
    D3D11_MAPPED_SUBRESOURCE mapped_resource = {0};
    result =
        context->Map(buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped_resource);
    assert(SUCCEEDED(result));

    // Update the data in the resource
    memcpy(mapped_resource.pData, data, bytes);

    // Reenable GPU access to data
    context->Unmap(buffer, 0);

    *bufferPtr = buffer;
}

// Vertex Shader Options:
void Direct3D11DeviceContext::loadVertexCB(uint8_t slot,
                                           const void* data,
                                           size_t bytes)
{
    if (bytes > 0)
    {
        loadConstantBuffer(&vertexCB[slot], data, bytes);
        context->VSSetConstantBuffers(slot, 1, &vertexCB[slot]);
    }
}

void Direct3D11DeviceContext::bindVertexTexture(
    uint8_t slot,
    const std::shared_ptr<Texture>& texture,
    SamplerSettings sampler)
{
    D3D11Texture* tex = reinterpret_cast<D3D11Texture*>(texture.get());
    ID3D11ShaderResourceView* srv = tex->getSRV();
    assert(srv);
    context->VSSetShaderResources(slot, 1, &srv);
    context->VSSetSamplers(slot, 1, &samplers[(int)sampler]);
}

// Pixel Shader Options:
void Direct3D11DeviceContext::loadPixelCB(uint8_t slot,
                                          const void* data,
                                          size_t bytes)
{
    if (bytes > 0)
    {
        loadConstantBuffer(&pixelCB[slot], data, bytes);
        context->PSSetConstantBuffers(slot, 1, &pixelCB[slot]);
    }
}
void Direct3D11DeviceContext::bindPixelTexture(
    uint8_t slot,
    const std::shared_ptr<Texture>& texture,
    SamplerSettings sampler)
{
    D3D11Texture* tex = reinterpret_cast<D3D11Texture*>(texture.get());
    ID3D11ShaderResourceView* srv = tex->getSRV();
    assert(srv);
    context->PSSetShaderResources(slot, 1, &srv);
    context->PSSetSamplers(slot, 1, &samplers[(int)sampler]);
}

void Direct3D11DeviceContext::draw(const Geometry* geometry,
                                   uint32_t instanceCount,
                                   VertexTopology vertexTopology)
{
    bool invalidateVertexBindings = false;
    for (int i = 0; i < BINDABLE_STREAM_COUNT; i++)
    {
        if (!geometry->vertexBuffers[i])
            continue;

        D3D11Buffer* vertexBuffer =
            reinterpret_cast<D3D11Buffer*>(geometry->vertexBuffers[i].get());
        if (vb_buffers[i] != vertexBuffer->getBuffer())
        {
            vb_buffers[i] = vertexBuffer->getBuffer();
            invalidateVertexBindings = true;
        }
    }

    if (invalidateVertexBindings)
    {
        context->IASetVertexBuffers(0, BINDABLE_STREAM_COUNT, vb_buffers,
                                    vb_strides, vb_offsets);
    }

    switch (vertexTopology)
    {
    case VertexTopology::TriangleList:
         context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
         break;
    case VertexTopology::LineList:
        context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINELIST);
        break;
    default:
        assert(false);
    }

    // Draw with Index Buffer
    if (geometry->indexBuffer)
    {
        D3D11Buffer* indexBuffer =
            reinterpret_cast<D3D11Buffer*>(geometry->indexBuffer.get());
        context->IASetIndexBuffer(indexBuffer->getBuffer(),
                                  DXGI_FORMAT_R32_UINT, 0);
        context->DrawIndexedInstanced(geometry->indexCount, instanceCount,
                                      geometry->indexOffset,
                                      geometry->vertexOffset, 0);
    }
    // Draw with no Index Buffer
    else
    {
        context->DrawInstanced(geometry->indexCount, instanceCount,
                               geometry->vertexOffset, 0);
    }
}

void Direct3D11DeviceContext::present()
{
    HRESULT result = swapchain->Present(1, 0);
    if (result == DXGI_ERROR_DEVICE_REMOVED ||
        result == DXGI_ERROR_DEVICE_RESET)
    {
        result = device->GetDeviceRemovedReason();
    }
    assert(SUCCEEDED(result));
}

} // namespace Graphics
} // namespace Engine