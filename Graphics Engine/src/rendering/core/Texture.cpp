#include "Texture.h"

#include <assert.h>

#if defined(_DEBUG)
#include "rendering/ImGui.h"
#endif

namespace Engine
{
namespace Graphics
{
Texture::Texture() = default;
Texture::Texture(ID3D11Texture2D* tex, UINT _width, UINT _height)
{
    texture = tex;
    width = _width;
    height = _height;

    shader_view = nullptr;
    depth_view = nullptr;
    target_view = nullptr;

    editable = false;
}

Texture::Texture(ID3D11Device* device, const D3D11_TEXTURE2D_DESC& desc)
{
    width = desc.Width;
    height = desc.Height;

    HRESULT result = device->CreateTexture2D(&desc, NULL, &texture);
    assert(SUCCEEDED(result));

    shader_view = nullptr;
    depth_view = nullptr;
    target_view = nullptr;

    editable = false;
}

Texture::~Texture()
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

#if defined(_DEBUG)
void Texture::displayImGui() const { displayImGui(256); }
void Texture::displayImGui(float display_width) const
{
    ImGui::Image((ImTextureID)(intptr_t)shader_view,
                 ImVec2(display_width, display_width * height / width));
}
#endif

} // namespace Graphics
} // namespace Engine