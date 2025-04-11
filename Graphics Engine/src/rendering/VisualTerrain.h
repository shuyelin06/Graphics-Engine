#pragma once

#include "core/Asset.h"
#include "datamodel/terrain/Terrain.h"
#include "resources/AssetBuilder.h"

namespace Engine {
using namespace Datamodel;

namespace Graphics {
// VisualTerrain Class:
// Stores rendering information for a terrain chunk.
class VisualTerrain {
  private:
    const Terrain* const terrain;

    // Stores my terrain meshes, in a format modeling our terrain.
    std::vector<Mesh*> chunk_meshes;
    std::vector<Mesh*> chunk_meshes_helper; // Used for copying data

    // Tracks the terrain center to determine if a mesh
    // update is needed
    int center_x, center_y, center_z;

  public:
    VisualTerrain(const Terrain* terrain);
    ~VisualTerrain();

    // Update the visual terrain's meshes by pulling the terrain's data
    void updateTerrainMeshes(MeshBuilder& builder);

    // Return the current meshes for rendering.
    const std::vector<Mesh*>& getTerrainMeshes();

  private:
    // Given index i,j,k returns the index in the vector
    // corresponding to what we would get if we had a 3D array
    int index3DVector(int i, int j, int k);
};

} // namespace Graphics
} // namespace Engine