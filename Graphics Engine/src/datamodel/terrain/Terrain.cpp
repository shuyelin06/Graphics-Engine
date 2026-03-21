#include "Terrain.h"

#include <assert.h>

#include "core/ThreadPool.h"

#include "math/Compute.h"
#include "math/PerlinNoise.h"
#include "math/Triangle.h"

#include "TerrainGenerator.h"

namespace Engine {
namespace Datamodel {
Terrain::Terrain() : Object(DMObjectTag::kTerrain) {
    generator = std::make_unique<TerrainGenerator>();

    setName("Terrain");
}

Terrain::~Terrain() = default;

void Terrain::propertyDisplay() {
#ifdef IMGUI_ENABLED
    generator->propertyDisplay();
#endif
}

// --- Accessors ---
TerrainGenerator& Terrain::getGenerator() const { return *generator; }

void Terrain::seed(unsigned int new_seed) {
    generator->seedGenerator(new_seed);
}

} // namespace Datamodel
} // namespace Engine