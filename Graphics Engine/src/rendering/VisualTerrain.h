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
    TerrainChunk* const terrain;

    bool markedToDestroy;

    Mesh* terrain_mesh;

    VisualTerrain(TerrainChunk* terrain, Mesh* terrain_mesh);

  public:
    ~VisualTerrain();

    bool markedForDestruction() const;
    void destroy();
};
} // namespace Graphics
} // namespace Engine