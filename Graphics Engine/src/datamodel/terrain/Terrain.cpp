#include "Terrain.h"

namespace Engine {
namespace Datamodel {
Terrain::Terrain()
    : Object(DMObjectTag::kTerrain),
      seed(&this->getDMHandle(), DMPropertyTag::kTerrain_Seed) {
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