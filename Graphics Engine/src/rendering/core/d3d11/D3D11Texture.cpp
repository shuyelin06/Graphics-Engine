#include "D3D11Texture.h"

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

D3D11Texture::D3D11Texture(ID3D11Device* device,
                           TextureLayout layout,
                           TextureUsage usage,
                           uint32_t width,
                           uint32_t height,
                           uint16_t slices,
                           uint8_t mips,
                           bool dynamic,
                           const void* src)
    : width(width)
    , height(height)
    , slices(slices)
    , mips(mips)
    , layout(layout)
    , dynamic(dynamic)
{
    const bool srv =
        (usage & TextureUsage::ShaderResource) == TextureUsage::ShaderResource;
    const bool depth =
        (usage & TextureUsage::DepthStencil) == TextureUsage::DepthStencil;
    const bool target =
        (usage & TextureUsage::RenderTarget) == TextureUsage::RenderTarget;

    HRESULT result;

    // Note: TextureUsage::RenderTarget used as a placeholder for the texture source format
    D3D11_TEXTURE2D_DESC desc = {};
    desc.Width = width;
    desc.Height = height;
    desc.MipLevels = mips;
    desc.ArraySize = slices;
    desc.Format = TextureLayoutToDXGI(layout, TextureUsage::RenderTarget);
    desc.SampleDesc.Count = 1;
    desc.Usage = dynamic ? D3D11_USAGE_DYNAMIC : D3D11_USAGE_DEFAULT;
    desc.CPUAccessFlags = dynamic ? D3D11_CPU_ACCESS_WRITE : 0;
    desc.BindFlags = 0;

    D3D11_SUBRESOURCE_DATA initData = {};
    D3D11_SUBRESOURCE_DATA* initDataPtr = nullptr;
    if (src != nullptr)
    {
        initData.pSysMem = src;
        initData.SysMemPitch = TextureLayoutByteSize(layout) * width;
        initDataPtr = &initData;
    }

    if (srv)
        desc.BindFlags |= D3D11_BIND_SHADER_RESOURCE;
    if (depth)
        desc.BindFlags |= D3D11_BIND_DEPTH_STENCIL;
    if (target)
        desc.BindFlags |= D3D11_BIND_RENDER_TARGET;

    result = device->CreateTexture2D(&desc, initDataPtr, &texture);
    assert(SUCCEEDED(result));

    if (srv)
    {
        D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
        srvDesc.Format =
            TextureLayoutToDXGI(layout, TextureUsage::ShaderResource);
        if (slices > 1)
        {
            srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DARRAY;
            srvDesc.Texture2DArray.MostDetailedMip = 0;
            srvDesc.Texture2DArray.MipLevels = mips;
            srvDesc.Texture2DArray.FirstArraySlice = 0;
            srvDesc.Texture2DArray.ArraySize = slices;
        }
        else
        {
            srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
            srvDesc.Texture2D.MostDetailedMip = 0;
            srvDesc.Texture2D.MipLevels = mips;
        }
        result =
            device->CreateShaderResourceView(texture, &srvDesc, &shader_view);
        assert(SUCCEEDED(result));
    }

    if (depth)
    {
        assert(mips == 1);
        D3D11_DEPTH_STENCIL_VIEW_DESC dsDesc = {};
        dsDesc.Format = TextureLayoutToDXGI(layout, TextureUsage::DepthStencil);
        if (slices > 1)
        {
            dsDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DARRAY;
            dsDesc.Texture2DArray.MipSlice = 0;
            dsDesc.Texture2DArray.FirstArraySlice = 0;
            dsDesc.Texture2DArray.ArraySize = slices;
        }
        else
        {
            dsDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
            dsDesc.Texture2D.MipSlice = 0;
        }
        result = device->CreateDepthStencilView(texture, &dsDesc, &depth_view);
        assert(SUCCEEDED(result));
    }

    if (target)
    {
        D3D11_RENDER_TARGET_VIEW_DESC targetDesc = {};
        targetDesc.Format =
            TextureLayoutToDXGI(layout, TextureUsage::RenderTarget);
        if (slices > 1)
        {
            targetDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DARRAY;
            targetDesc.Texture2DArray.MipSlice = 0;
            targetDesc.Texture2DArray.FirstArraySlice = 0;
            targetDesc.Texture2DArray.ArraySize = slices;
        }
        else
        {
            targetDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
            targetDesc.Texture2D.MipSlice = 0;
        }
        result =
            device->CreateRenderTargetView(texture, &targetDesc, &target_view);
        assert(SUCCEEDED(result));
    }

    ready = true;
}
D3D11Texture::~D3D11Texture()
{
    if (texture)
        texture->Release();
    if (depth_view)
        depth_view->Release();
    if (shader_view)
        shader_view->Release();
    if (target_view)
        target_view->Release();
}

void D3D11Texture::update(ID3D11DeviceContext* context,
                          uint8_t slice,
                          const void* initData,
                          size_t bytes)
{
    const size_t rowPitch = TextureLayoutByteSize(layout) * width;
    const size_t depthPitch = rowPitch * height;
    assert(bytes == depthPitch);

    if (dynamic)
    {
        // Write to my texture using Map / Unmap.
        D3D11_MAPPED_SUBRESOURCE sr;
        HRESULT result =
            context->Map(texture, slice, D3D11_MAP_WRITE_DISCARD, 0, &sr);
        assert(SUCCEEDED(result));
        assert(sr.RowPitch == rowPitch && sr.DepthPitch == depthPitch);

        uint8_t* dest = reinterpret_cast<uint8_t*>(sr.pData);
        const uint8_t* src = reinterpret_cast<const uint8_t*>(initData);

        // We need to copy row-by-row, because while rows are aligned, there
        // may be padding after each row that we're not aware about.
        for (UINT y = 0; y < height; ++y)
        {
            memcpy(dest + y * rowPitch, src + y * rowPitch, rowPitch);
        }

        context->Unmap(texture, 0);
    }
    else
    {
        context->UpdateSubresource(texture, slice, nullptr, initData, rowPitch,
                                   depthPitch);
    }
}

#if defined(IMGUI_ENABLED)
static const char* toString(bool status) { return status ? "T" : "F"; }
static const char* toString(TextureLayout layout)
{
    switch (layout)
    {
    case TextureLayout::R8G8B8A8_UNORM_SGRB:
        return "R8G8B8A8_UNORM_SGRB";
    case TextureLayout::R8G8B8A8_UNORM:
        return "R8G8B8A8_UNORM";
    case TextureLayout::R32_FLOAT:
        return "R32_FLOAT";
    case TextureLayout::R24_UNORM_G8_UINT:
        return "R24_UNORM_G8_UINT";
    case TextureLayout::UNKNOWN:
        return "UNKNOWN";
    }
}

void D3D11Texture::doImgui() const
{
    if (texture == nullptr)
    {
        ImGui::Text("No Texture Available");
        return;
    }
    else if (ready.load() == false)
    {
        ImGui::Text("Texture not ready yet");
        return;
    }

    ImGui::Text("Texture %s (Editable: %s)", "--", // TODO
                toString(dynamic));
    ImGui::Text("Format: %s", toString(layout));
    ImGui::Text("Width: %u, Height: %u", width, height);
    ImGui::Text("Mips: %u, Slices: %u", mips, slices);
    ImGui::Text("Shader View: %s, Depth View: %s, Target View: %s",
                toString(shader_view), toString(depth_view),
                toString(target_view));

    constexpr int kTextureDisplayWidth = 256;
    ImGui::Image(
        (ImTextureID)(intptr_t)shader_view,
        ImVec2(kTextureDisplayWidth, kTextureDisplayWidth * height / width));
}
#endif

} // namespace Graphics
} // namespace Engine