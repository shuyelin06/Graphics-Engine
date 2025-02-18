#include "VisualTerrain.h"

namespace Engine {
namespace Graphics {
VisualTerrain::VisualTerrain(TerrainChunk* _terrain, Mesh* _terrain_mesh)
    : terrain(_terrain) {
    markedToDestroy = false;

    terrain_mesh = _terrain_mesh;
}

VisualTerrain::~VisualTerrain() = default;

void VisualTerrain::destroy() { markedToDestroy = true; }

bool VisualTerrain::markedForDestruction() const {
    return markedToDestroy;
}

} // namespace Graphics
} // namespace Engine