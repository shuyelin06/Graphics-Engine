#include "TextureManager.h"

#include <assert.h>

namespace Engine {
namespace Graphics {
TextureManager::TextureManager(ID3D11Device* _device) : textures() {
    device = _device;
}

// GetTexture:
// Returns a texture by name from the TextureManager.
Texture* TextureManager::getTexture(const std::string& name) {
    if (textures.contains(name))
        return textures[name];
    else
        return nullptr;
}

// Texture Generation
// Built-in texture generation methods for creating important
// textures in the rendering pipeline.
Texture* TextureManager::createRenderTexture(const std::string& name,
                                             UINT width, UINT height) {
    HRESULT result;
    Texture* texture = new Texture(width, height);

    // Create my texture. This is a texture with RGBA channels for color.
    D3D11_TEXTURE2D_DESC tex_desc = {};
    tex_desc.Width = width;
    tex_desc.Height = height;
    tex_desc.MipLevels = 1;
    tex_desc.ArraySize = 1;
    tex_desc.MipLevels = 1;
    tex_desc.ArraySize = 1;
    tex_desc.SampleDesc.Count = 1;
    tex_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    tex_desc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;

    result = device->CreateTexture2D(&tex_desc, NULL, &texture->texture);
    assert(SUCCEEDED(result));

    // Add a render target view for my texture.
    result = device->CreateRenderTargetView(texture->texture, 0,
                                            &texture->target_view);
    assert(SUCCEEDED(result));

    // Add a shader resource view for my texture.
    D3D11_SHADER_RESOURCE_VIEW_DESC resource_view_desc = {};
    resource_view_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    resource_view_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    resource_view_desc.Texture2D.MostDetailedMip = 0;
    resource_view_desc.Texture2D.MipLevels = 1;

    result = device->CreateShaderResourceView(
        texture->texture, &resource_view_desc, &texture->shader_view);
    assert(SUCCEEDED(result));

    return texture;
}

// DepthTexture: Stores depth stencil values. Used in the main render's
// z-testing
Texture* TextureManager::createDepthTexture(const std::string& name, UINT width,
                                            UINT height) {
    HRESULT result;
    Texture* texture = new Texture(width, height);

    // Create my texture
    // 24 Bits for Depth, 8 Bits for Stencil
    D3D11_TEXTURE2D_DESC tex_desc = {};
    tex_desc.Width = width;
    tex_desc.Height = height;
    tex_desc.MipLevels = 1;
    tex_desc.ArraySize = 1;
    tex_desc.MipLevels = 1;
    tex_desc.ArraySize = 1;
    tex_desc.SampleDesc.Count = 1;
    tex_desc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    tex_desc.BindFlags = D3D11_BIND_DEPTH_STENCIL;

    result = device->CreateTexture2D(&tex_desc, NULL, &texture->texture);
    assert(SUCCEEDED(result));

    // Create my depth view
    D3D11_DEPTH_STENCIL_VIEW_DESC desc_stencil = {};
    desc_stencil.Format =
        DXGI_FORMAT_D24_UNORM_S8_UINT; // Same format as texture
    desc_stencil.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DMS;

    result = device->CreateDepthStencilView(texture->texture, &desc_stencil,
                                            &texture->depth_view);
    assert(SUCCEEDED(result));

    bool registered = registerTexture(name, texture);

    if (registered)
        return texture;
    else
        return nullptr;
}

// ShadowTexture: Stores light shadow map values.
// Used in the light shadow map atlas
Texture* TextureManager::createShadowTexture(const std::string& name,
                                             UINT width, UINT height) {
    HRESULT result;
    Texture* texture = new Texture(width, height);

    // Create my texture resource. This will have
    // 24 Bits for R Channel (depth), 8 Bits for G Channel (stencil).
    // The resource will be able to be accessed as a depth stencil and
    // shader resource.
    D3D11_TEXTURE2D_DESC tex_desc = {};
    tex_desc.Width = width;
    tex_desc.Height = height;
    tex_desc.MipLevels = tex_desc.ArraySize = 1;
    tex_desc.Format = DXGI_FORMAT_R24G8_TYPELESS;
    tex_desc.SampleDesc.Count = 1;
    tex_desc.Usage = D3D11_USAGE_DEFAULT;
    tex_desc.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;
    tex_desc.CPUAccessFlags = 0;
    tex_desc.MiscFlags = 0;

    result = device->CreateTexture2D(&tex_desc, NULL, &texture->texture);
    assert(SUCCEEDED(result));

    // Initialize a depth stencil view, to allow the texture to be used
    // as a depth buffer.
    // DXGI_FORMAT_D24_UNORM_S8_UINT specifies 24 bits for depth, 8 bits for
    // stencil
    D3D11_DEPTH_STENCIL_VIEW_DESC depth_stencil_view_desc = {};
    depth_stencil_view_desc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    depth_stencil_view_desc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
    depth_stencil_view_desc.Texture2D.MipSlice = 0;

    result = device->CreateDepthStencilView(
        texture->texture, &depth_stencil_view_desc, &texture->depth_view);
    assert(SUCCEEDED(result));

    // Initialize a shader resource view, so that the texture data
    // can be sampled in the shader.
    // DXGI_FORMAT_R24_UNORM_X8_TYPELESS specifies 24 bits in the R channel
    // UNORM (0.0f -> 1.0f), and 8 bits to be ignored
    D3D11_SHADER_RESOURCE_VIEW_DESC shader_resource_view_description = {};
    shader_resource_view_description.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
    shader_resource_view_description.ViewDimension =
        D3D11_SRV_DIMENSION_TEXTURE2D;
    shader_resource_view_description.Texture2D.MostDetailedMip = 0;
    shader_resource_view_description.Texture2D.MipLevels = 1;

    result = device->CreateShaderResourceView(texture->texture,
                                              &shader_resource_view_description,
                                              &texture->shader_view);
    assert(SUCCEEDED(result));

    // Register my texture and return it
    bool registered = registerTexture(name, texture);

    if (registered)
        return texture;
    else
        return nullptr;
}

// RegisterTexture:
// Registers a texture into the textures map.
// True on success, false on failure.
bool TextureManager::registerTexture(const std::string& name,
                                     Texture* texture) {
    if (!textures.contains(name)) {
        textures[name] = texture;
        return true;
    } else
        return false;
}

} // namespace Graphics
} // namespace Engine