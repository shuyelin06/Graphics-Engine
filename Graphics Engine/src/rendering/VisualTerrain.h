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
    std::vector<Mesh*> tree_meshes;

    bool markedToDestroy;

    VisualTerrain(TerrainChunk* terrain, MeshBuilder* mesh_builder);

  public:
    ~VisualTerrain();

    bool markedForDestruction() const;
    void destroy();

  private:
    Mesh* generateTerrainMesh(MeshBuilder& builder);
    Mesh* generateTreeMesh(MeshBuilder& builder, const Vector2& location);
};

} // namespace Graphics
} // namespace Engine