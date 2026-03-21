#include "Terrain.h"

namespace Engine {
namespace Datamodel {
Terrain::Terrain() : Object("Terrain"), seed(&this->getDMHandle(), "Seed") {
    setName("Terrain");
}

Terrain::~Terrain() = default;

void Terrain::propertyDisplay() {
#ifdef IMGUI_ENABLED
    static int propSeed = 0;
    ImGui::SliderInt("Terrain Seed", &propSeed, 0, 0xFFFF);
    if (propSeed != seed.readProperty()) {
        seed.writeProperty(propSeed);
    }
#endif
}

} // namespace Datamodel
} // namespace Engine