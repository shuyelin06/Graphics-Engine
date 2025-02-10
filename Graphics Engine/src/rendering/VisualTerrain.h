#pragma once

#include "core/Asset.h"
#include "datamodel/Terrain.h"

namespace Engine {
using namespace Datamodel;

namespace Graphics {
// VisualTerrain Class:
// Stores rendering information for a terrain chunk.
class VisualTerrain {
    friend class VisualSystem;

  private:
    Terrain* const terrain;

    Mesh* terrain_mesh;

    VisualTerrain(Terrain* terrain, Mesh* terrain_mesh);

  public:
    ~VisualTerrain();
};
} // namespace Graphics
} // namespace Engine