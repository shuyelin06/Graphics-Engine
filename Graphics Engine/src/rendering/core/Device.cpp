#include "Device.h"

#include <assert.h>
#include <utility>

#include "RenderSettings.h"

#include "rendering/Direct3D11.h"

namespace Engine
{
namespace Graphics
{
// We will use render target to describe the source texture description format
static inline DXGI_FORMAT TextureLayoutToDXGI(const TextureLayout layout,
                                              const TextureUsage usageDesc)
{
    switch (layout)
    {
    case TextureLayout::R8G8B8A8_UNORM_SGRB:
        return DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
    case TextureLayout::R8G8B8A8_UNORM:
        return DXGI_FORMAT_R8G8B8A8_UNORM;
    case TextureLayout::R32_FLOAT:
        return DXGI_FORMAT_R32_FLOAT;
    case TextureLayout::R24_UNORM_G8_UINT: {
        switch (usageDesc)
        {
        case TextureUsage::RenderTarget:
            return DXGI_FORMAT_R24G8_TYPELESS;
        case TextureUsage::DepthStencil:
            return DXGI_FORMAT_D24_UNORM_S8_UINT;
        case TextureUsage::ShaderResource:
            return DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
        }
    }
    }

    return DXGI_FORMAT_UNKNOWN;
}

struct DeviceImpl
{
    ID3D11Device* device;
};
struct DeviceContextImpl
{
    ID3D11Device* device = nullptr;
    ID3D11DeviceContext* context = nullptr;
    ID3D11RenderTargetView* target = nullptr;
    IDXGISwapChain* swapchain = nullptr;

    ID3D11Buffer* vertexCB[kVertexConstantBufferMax] = {nullptr};
    ID3D11Buffer* pixelCB[kVertexConstantBufferMax] = {nullptr};
};

void InitializeGraphicsAPI(HWND window,
                           std::unique_ptr<Device>& outDevice,
                           std::unique_ptr<DeviceContext>& outContext)
{
    outDevice = std::make_unique<Device>();
    outDevice->mImpl = std::make_unique<DeviceImpl>();
    outContext = std::make_unique<DeviceContext>();
    outContext->mImpl = std::make_unique<DeviceContextImpl>();

    auto& device = outDevice->mImpl;
    auto& context = outContext->mImpl;

    RECT rect;
    GetClientRect(window, &rect);
    const UINT width = rect.right - rect.left;
    const UINT height = rect.bottom - rect.top;

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
        0, // Flags
        NULL, 0, D3D11_SDK_VERSION, &swap_chain_descriptor, &context->swapchain,
        &device->device, &feature_level, &context->context);
    assert(S_OK == result && device->device && context->swapchain &&
           context->context);

    context->device = device->device;

    // Create my screen target with the swap chain's frame buffer. This
    // will store my output image.
    ID3D11Texture2D* texture;

    result = context->swapchain->GetBuffer(0, __uuidof(ID3D11Texture2D),
                                           (void**)&texture);
    assert(SUCCEEDED(result));

    result =
        device->device->CreateRenderTargetView(texture, 0, &context->target);
    assert(SUCCEEDED(result));

    // Free frame buffer (no longer needed)
    texture->Release();
}

Device::Device() = default;
Device::~Device()
{
    if (mImpl)
    {
        mImpl->device->Release();
    }
}

ID3D11Device* Device::getDevice() { return mImpl->device; }

std::shared_ptr<Texture> Device::createTexture(TextureLayout layout,
                                               TextureUsage usage,
                                               unsigned int width,
                                               unsigned int height,
                                               unsigned int mips,
                                               const char* debugName,
                                               bool dynamic)
{
    auto& device = mImpl->device;

    const bool srv =
        (usage & TextureUsage::ShaderResource) == TextureUsage::ShaderResource;
    const bool depth =
        (usage & TextureUsage::DepthStencil) == TextureUsage::DepthStencil;
    const bool target =
        (usage & TextureUsage::RenderTarget) == TextureUsage::RenderTarget;

    std::shared_ptr<Texture> texture = std::make_shared<Texture>();
    texture->width = width;
    texture->height = height;
    texture->mips = mips;
    texture->layout = layout;
    texture->editable = dynamic;
    texture->debugName = debugName;

    HRESULT result;

    // Note: TextureUsage::RenderTarget used as a placeholder for the texture source format
    D3D11_TEXTURE2D_DESC desc = {};
    desc.Width = width;
    desc.Height = height;
    desc.MipLevels = mips;
    desc.ArraySize = 1;
    desc.Format = TextureLayoutToDXGI(layout, TextureUsage::RenderTarget);
    desc.SampleDesc.Count = 1;
    desc.Usage = dynamic ? D3D11_USAGE_DYNAMIC : D3D11_USAGE_DEFAULT;
    desc.CPUAccessFlags = dynamic ? D3D11_CPU_ACCESS_WRITE : 0;
    desc.BindFlags = 0;

    if (srv)
        desc.BindFlags |= D3D11_BIND_SHADER_RESOURCE;
    if (depth)
        desc.BindFlags |= D3D11_BIND_DEPTH_STENCIL;
    if (target)
        desc.BindFlags |= D3D11_BIND_RENDER_TARGET;

    result = device->CreateTexture2D(&desc, nullptr, &texture->texture);

    if (!SUCCEEDED(result))
        return nullptr;

    if (srv)
    {
        D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
        srvDesc.Format =
            TextureLayoutToDXGI(layout, TextureUsage::ShaderResource);
        srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Texture2D.MostDetailedMip = 0;
        srvDesc.Texture2D.MipLevels = mips;
        result = device->CreateShaderResourceView(texture->texture, &srvDesc,
                                                  &texture->shader_view);
        if (!SUCCEEDED(result))
            return nullptr;
    }

    if (depth)
    {
        assert(mips == 1);
        D3D11_DEPTH_STENCIL_VIEW_DESC dsDesc = {};
        dsDesc.Format = TextureLayoutToDXGI(layout, TextureUsage::DepthStencil);
        dsDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
        dsDesc.Texture2D.MipSlice = 0;
        result = device->CreateDepthStencilView(texture->texture, &dsDesc,
                                                &texture->depth_view);
        if (!SUCCEEDED(result))
            return nullptr;
    }

    if (target)
    {
        result = device->CreateRenderTargetView(texture->texture, 0,
                                                &texture->target_view);
        if (!SUCCEEDED(result))
            return nullptr;
    }

    texture->ready = true;
    return texture;
}

DeviceContext::DeviceContext() = default;
DeviceContext::~DeviceContext()
{
    if (mImpl)
    {
        mImpl->context->Release();
        mImpl->target->Release();
        mImpl->swapchain->Release();
        for (uint8_t slot = 0; slot < kVertexConstantBufferMax; slot++)
        {
            if (mImpl->vertexCB[slot])
                mImpl->vertexCB[slot]->Release();
        }
        for (uint8_t slot = 0; slot < kPixelConstantBufferMax; slot++)
        {
            if (mImpl->pixelCB[slot])
                mImpl->pixelCB[slot]->Release();
        }
    }
}

ID3D11DeviceContext* DeviceContext::getContext() { return mImpl->context; }
ID3D11RenderTargetView* DeviceContext::getRenderTarget()
{
    return mImpl->target;
}

static bool loadConstantBuffer(DeviceContextImpl& impl,
                               ID3D11Buffer** buffer,
                               const void* data,
                               size_t bytes)
{
    // Do nothing if CB has nothing
    if (bytes == 0)
        return false;

    auto& device = impl.device;
    auto& context = impl.context;

    // If the buffer resource has never been created before, or the current
    // resource is too small for mapping / unmapping, create a new one.
    if (*buffer == nullptr)
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
        HRESULT result = device->CreateBuffer(&buff_desc, NULL, buffer);
        assert(SUCCEEDED(result));
    }

    // Perform resource renaming to update buffer data
    // Disable GPU access to data and obtain the my constant buffer resource
    D3D11_MAPPED_SUBRESOURCE mapped_resource = {0};
    context->Map(*buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped_resource);

    // Update the data in the resource
    memcpy(mapped_resource.pData, data, bytes);

    // Reenable GPU access to data
    context->Unmap(*buffer, 0);

    return true;
}

void DeviceContext::loadVertexCB(uint8_t slot, const void* data, size_t bytes)
{
    const bool success =
        loadConstantBuffer(*mImpl, &mImpl->vertexCB[slot], data, bytes);
    if (success)
        mImpl->context->VSSetConstantBuffers(slot, 1, &mImpl->vertexCB[slot]);
}

void DeviceContext::loadPixelCB(uint8_t slot, const void* data, size_t bytes)
{
    const bool success =
        loadConstantBuffer(*mImpl, &mImpl->pixelCB[slot], data, bytes);
    if (success)
        mImpl->context->PSSetConstantBuffers(slot, 1, &mImpl->pixelCB[slot]);
}

void DeviceContext::present() { mImpl->swapchain->Present(1, 0); }

} // namespace Graphics
} // namespace Engine