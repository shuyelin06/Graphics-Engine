#include "Device.h"

#include "rendering/Direct3D11.h"

namespace Engine
{
namespace Graphics
{
static inline DXGI_FORMAT TextureLayoutToDXGI(TextureLayout layout)
{
    switch (layout)
    {
    case TextureLayout::R8G8B8A8_UNORM:
        return DXGI_FORMAT_R8G8B8A8_UNORM;
    case TextureLayout::R32_FLOAT:
        return DXGI_FORMAT_R32_FLOAT;
    }
    return DXGI_FORMAT_UNKNOWN;
}

Device::Device(ID3D11Device* device)
    : device(device)
{
}
Device::~Device() = default;

std::shared_ptr<Texture> Device::createTexture(TextureLayout layout,
                                               unsigned int width,
                                               unsigned int height,
                                               unsigned int mips,
                                               bool dynamic)
{
    D3D11_TEXTURE2D_DESC desc = {};
    desc.Width = width;
    desc.Height = height;
    desc.MipLevels = mips;
    desc.ArraySize = 1;
    desc.Format = TextureLayoutToDXGI(layout);
    desc.SampleDesc.Count = 1;
    desc.Usage = dynamic ? D3D11_USAGE_DYNAMIC : D3D11_USAGE_DEFAULT;
    desc.CPUAccessFlags = dynamic ? D3D11_CPU_ACCESS_WRITE : 0;
    desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

    ID3D11Texture2D* resource = nullptr;
    HRESULT result = device->CreateTexture2D(&desc, nullptr, &resource);

    if (!SUCCEEDED(result))
        return nullptr;

    ID3D11ShaderResourceView* srv = nullptr;
    D3D11_SHADER_RESOURCE_VIEW_DESC shader_view = {};
    shader_view.Format = TextureLayoutToDXGI(layout);
    shader_view.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    shader_view.Texture2D.MostDetailedMip = 0;
    shader_view.Texture2D.MipLevels = mips;
    result = device->CreateShaderResourceView(resource, &shader_view, &srv);

    if (!SUCCEEDED(result))
        return nullptr;

    std::shared_ptr<Texture> texture = std::make_shared<Texture>();
    texture->width = width;
    texture->height = height;
    texture->mips = mips;
    texture->layout = layout;
    texture->editable = dynamic;

    texture->texture = resource;
    texture->shader_view = srv;

    texture->ready = true;

    return texture;
}

DeviceContext::DeviceContext(ID3D11DeviceContext* context)
    : context(context)
{
}

} // namespace Graphics
} // namespace Engine