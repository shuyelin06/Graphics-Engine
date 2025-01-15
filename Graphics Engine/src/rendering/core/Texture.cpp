#include "Texture.h"

#if defined(_DEBUG)
#include "rendering/ImGui.h"
#endif

namespace Engine {
namespace Graphics {

#if defined(_DEBUG)
void Texture::displayImGui() {
    ImGui::Image((ImTextureID) (intptr_t) view,
                 ImVec2(width, height));
}
#endif

} // namespace Graphics
} // namespace Engine