#include "Texture.h"

#if defined(_DEBUG)
#include "rendering/ImGui.h"
#endif

namespace Engine {
namespace Graphics {

Texture::Texture(UINT _width, UINT _height) {
    width = _width;
    height = _height;
}
Texture::~Texture() {
    if (texture != nullptr)
        texture->Release();
    if (shader_view != nullptr)
        shader_view->Release();
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