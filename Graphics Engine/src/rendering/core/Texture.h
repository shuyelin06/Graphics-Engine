#pragma once

#include "rendering/Direct3D11.h"

typedef unsigned int UINT;

namespace Engine {
namespace Graphics {

// Texture Struct:
// Represents a texture that can be uploaded to the GPU.
struct Texture {
    ID3D11Texture2D* texture;

    // Different views for the texture. NULL if
    // uninitialized.
    ID3D11ShaderResourceView* shader_view;
    ID3D11DepthStencilView* depth_view;
    ID3D11RenderTargetView* target_view;

    UINT width;
    UINT height;

    Texture(UINT width, UINT height);
    ~Texture();

#if defined(_DEBUG)
    void displayImGui() const;
    void displayImGui(float width) const;
#endif
};

} // namespace Graphics
} // namespace Engine