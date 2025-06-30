#pragma once

#include "rendering/Direct3D11.h"

#include "math/Color.h"

typedef unsigned int UINT;

namespace Engine {
using namespace Math;

namespace Graphics {

// Texture Struct:
// Represents a texture that can be uploaded to the GPU.
struct Texture {
    // GPU handle to the texture
    ID3D11Texture2D* texture;

    // Texture descriptions
    UINT width, height; // Pixel width, height
    bool editable;      // Can the texture be edited?

    // Different views for the texture. NULL if uninitialized.
    ID3D11ShaderResourceView* shader_view;
    ID3D11DepthStencilView* depth_view;
    ID3D11RenderTargetView* target_view;

  public:
    Texture(ID3D11Texture2D* tex, UINT width, UINT height);
    Texture(ID3D11Device* device, const D3D11_TEXTURE2D_DESC& desc);
    ~Texture();

    // View Initialization
    void createShaderResourceView(ID3D11Device* device,
                                  D3D11_SHADER_RESOURCE_VIEW_DESC& desc);
    void createDepthStencilView(ID3D11Device* device,
                                D3D11_DEPTH_STENCIL_VIEW_DESC& desc);
    void createRenderTargetView(ID3D11Device* device);

    // Texture Operations
    void VSBindResource(ID3D11DeviceContext* context, unsigned int slot) const;
    void PSBindResource(ID3D11DeviceContext* context, unsigned int slot) const;
    void clearAsRenderTarget(ID3D11DeviceContext* context,
                             const Color& color) const;
    void clearAsDepthStencil(ID3D11DeviceContext* context) const;

#if defined(_DEBUG)
    void displayImGui() const;
    void displayImGui(float width) const;
#endif
};

// RenderTargetTexture:
// Represents a texture that can also be used as a render target.
struct RenderTargetTexture : public Texture {};

// Depth Stencil Texture:
// Represents a texture that can also be used as a depth stencil
struct DepthStencilTexture : public Texture {};

} // namespace Graphics
} // namespace Engine