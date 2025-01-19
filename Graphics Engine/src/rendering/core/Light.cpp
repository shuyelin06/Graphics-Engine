#include "Light.h"

#include <assert.h>

namespace Engine {
namespace Graphics {
// Constructor:
// Initializes a texture resource for use in the shadow mapping. The
// device is needed to intialize
Light::Light(ID3D11Device* device, ShadowMapQuality quality) : Camera() {
    color = Color(1.0f, 1.0f, 1.0f);

    // Create the shadowmap texture, a 2D texture storing depth information
    // for some light. This texture can be used in the pixel shader to see if a
    // pixel is "in view" of the light or not, so we know if it is in shadow or
    // not. Note that we must set the format to "TYPELESS", so that the texture
    // format support many binding flags- in other words, be reinterpreted as
    // different data formats.
    shadow_map = NULL;

    D3D11_TEXTURE2D_DESC tex_desc = {};
    tex_desc.Width = quality;
    tex_desc.Height = quality;
    tex_desc.MipLevels = tex_desc.ArraySize = 1;
    tex_desc.Format = DXGI_FORMAT_R24G8_TYPELESS; // 24 Bits for R Channel, 8
                                                  // Bits for G Channel
    tex_desc.SampleDesc.Count = 1;
    tex_desc.Usage = D3D11_USAGE_DEFAULT;
    tex_desc.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;
    tex_desc.CPUAccessFlags = 0;
    tex_desc.MiscFlags = 0;

    device->CreateTexture2D(&tex_desc, NULL, &shadow_map);
    assert(shadow_map != NULL);

    shadowmap_width = quality;
    shadowmap_height = quality;

    // Initialize a depth stencil view, to allow the texture to be used
    // as a depth buffer. This way, we can render the scene to automatically
    // store
    // the depth in this texture.
    // DXGI_FORMAT_D24_UNORM_S8_UINT specifies 24 bits for depth, 8 bits for
    // stencil
    depth_stencil_view = NULL;

    D3D11_DEPTH_STENCIL_VIEW_DESC depth_stencil_view_desc = {};
    depth_stencil_view_desc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    depth_stencil_view_desc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
    depth_stencil_view_desc.Texture2D.MipSlice = 0;

    device->CreateDepthStencilView(shadow_map, &depth_stencil_view_desc,
                                   &depth_stencil_view);
    assert(depth_stencil_view != NULL);

    // Create viewport to match the texture size, so that we can render to the
    // texture.
    viewport = {};
    viewport.Width = quality;
    viewport.Height = quality;
    viewport.MinDepth = 0.0f;
    viewport.MaxDepth = 1.0f;
    viewport.TopLeftX = 0;
    viewport.TopLeftY = 0;

    // Create a shader resource view, so that the texture data
    // can be sampled in the shader.
    // DXGI_FORMAT_R24_UNORM_X8_TYPELESS specifies 24 bits in the R channel
    // UNORM (0.0f -> 1.0f), and 8 bits to be ignored
    shader_resource_view = NULL;

    D3D11_SHADER_RESOURCE_VIEW_DESC shader_resource_view_description = {};
    shader_resource_view_description.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
    shader_resource_view_description.ViewDimension =
        D3D11_SRV_DIMENSION_TEXTURE2D;
    shader_resource_view_description.Texture2D.MostDetailedMip = 0;
    shader_resource_view_description.Texture2D.MipLevels = 1;

    device->CreateShaderResourceView(
        shadow_map, &shader_resource_view_description, &shader_resource_view);
    assert(shader_resource_view != NULL);

    // With our shader view, we also need to specify how the texture is to be
    // sampled.
    // For a shadowmap, we make it so that any readings "outside" the texture
    // are automatically outside of the light's view (in other words, 0).
    sampler_state = NULL;

    D3D11_SAMPLER_DESC sampler_desc = {};
    sampler_desc.Filter = D3D11_FILTER_ANISOTROPIC;
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

    device->CreateSamplerState(&sampler_desc, &sampler_state);
    assert(sampler_state != NULL);
}

Light::~Light() = default;

Color& Light::getColor() { return color; }

D3D11_VIEWPORT& Light::getViewport() { return viewport; }

ID3D11DepthStencilView*& Light::getDepthView() { return depth_stencil_view; }

ID3D11ShaderResourceView*& Light::getShaderView() {
    return shader_resource_view;
}

ID3D11SamplerState*& Light::getSampler() { return sampler_state; }

UINT Light::getWidth() { return shadowmap_width; }
UINT Light::getHeight() { return shadowmap_height; }

const Matrix4 Light::getProjectionMatrix(void) const {
    Matrix4 projection_matrix = Matrix4();

    const float left = -20.f;
    const float right = 20.f;
    const float bottom = -20.f;
    const float top = 20.f;
    const float z_near = 5.f;
    const float z_far = 200.f;

    projection_matrix[0][0] = 2 / (right - left);
    projection_matrix[1][1] = 2 / (top - bottom);
    projection_matrix[2][2] = 1 / (z_far - z_near);
    projection_matrix[3][3] = 1;

    projection_matrix[3][0] = - (right + left) / (right - left);
    projection_matrix[3][1] = - (top + bottom) / (top - bottom);
    projection_matrix[3][2] = - (z_near) / (z_far - z_near);

    return projection_matrix;
}

} // namespace Graphics
} // namespace Engine