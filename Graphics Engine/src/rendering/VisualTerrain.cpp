#include "VisualTerrain.h"

namespace Engine {
namespace Graphics {
VisualTerrain::VisualTerrain(Terrain* _terrain, Mesh* _terrain_mesh)
    : terrain(_terrain) {
    terrain_mesh = _terrain_mesh;
}

VisualTerrain::~VisualTerrain() = default;

} // namespace Graphics
} // namespace Engine