#pragma once

#include "core/Asset.h"
#include "core/MeshBuilder.h"
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
    Mesh* terrain_mesh;

    bool markedToDestroy;

    VisualTerrain(TerrainChunk* terrain, MeshBuilder* mesh_builder);

  public:
    ~VisualTerrain();

    bool markedForDestruction() const;
    void destroy();

  private:
    Mesh* generateTerrainMesh(MeshBuilder& builder);
};
} // namespace Graphics
} // namespace Engine