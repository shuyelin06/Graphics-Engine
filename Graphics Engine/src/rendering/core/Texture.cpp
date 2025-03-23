#include "Texture.h"

#include <assert.h>

#if defined(_DEBUG)
#include "rendering/ImGui.h"
#endif

namespace Engine {
namespace Graphics {

Texture::Texture(UINT _width, UINT _height) {
    width = _width;
    height = _height;

    texture = nullptr;
    shader_view = nullptr;
    depth_view = nullptr;
    target_view = nullptr;
}

Texture::~Texture() {
    if (texture != nullptr)
        texture->Release();
    if (shader_view != nullptr)
        shader_view->Release();
}

void Texture::clearAsRenderTarget(ID3D11DeviceContext* context,
                                  const Color& color) {
    assert(target_view != nullptr);

    const float color_arr[4] = {color.r, color.g, color.b, 1.f};
    context->ClearRenderTargetView(target_view, color_arr);
}

void Texture::bindAsRenderTarget(ID3D11DeviceContext* context) {
    assert(target_view != nullptr);

    // Set render target
    context->OMSetRenderTargets(1, &target_view, nullptr);

    // Create my viewport
    const D3D11_VIEWPORT viewport = {0.0f,          0.0f, (float)width,
                                     (float)height, 0.0f, 1.0f};
    context->RSSetViewports(1, &viewport);
}

void Texture::clearAsDepthStencil(ID3D11DeviceContext* context) {
    context->ClearDepthStencilView(depth_view, D3D11_CLEAR_DEPTH, 1.0f, 0);
}
void Texture::bindAsDepthStencil(ID3D11DeviceContext* context) {
    // Set depth stencil
    context->OMSetRenderTargets(1, nullptr, depth_view);

    // Create my viewport
    const D3D11_VIEWPORT viewport = {0.0f,          0.0f, (float)width,
                                     (float)height, 0.0f, 1.0f};
    context->RSSetViewports(1, &viewport);
}

#if defined(_DEBUG)
void Texture::displayImGui() const { displayImGui(256); }
void Texture::displayImGui(float display_width) const {
    ImGui::Image((ImTextureID)(intptr_t)shader_view,
                 ImVec2(display_width, display_width * height / width));
}
#endif

} // namespace Graphics
} // namespace Engine