#pragma once

#include "core/Asset.h"
#include "datamodel/terrain/Terrain.h"
#include "datamodel/terrain/TerrainCallback.h"
#include "resources/AssetBuilder.h"

namespace Engine {
using namespace Datamodel;

namespace Graphics {
// VisualTerrainCallback:
// Given a terrain chunk reload call, regenerates the chunk's mesh
class VisualTerrainCallback : public TerrainCallback {
  private:
    ID3D11Device* device;
    Mesh* output_mesh;
    bool dirty;

    // Synchronization
    std::mutex mutex;

  public:
    VisualTerrainCallback();

    void initialize(ID3D11Device* device);

    Mesh* extractMesh();
    bool isDirty();

    void reloadTerrainData(const TerrainChunk* chunk_data);
};

// VisualTerrain Class:
// Stores rendering information for a terrain chunk.
class VisualTerrainCallback;
class VisualTerrain {
  private:
    Terrain* terrain;

    // Output Meshes
    std::vector<Mesh*> output_meshes;

    // Stores my most recent terrain meshes, in a format modeling our terrain.
    Mesh* chunk_meshes[TERRAIN_CHUNK_COUNT][TERRAIN_CHUNK_COUNT]
                      [TERRAIN_CHUNK_COUNT];
    // Stores callback functions that may update with new terrain data
    VisualTerrainCallback callbacks[TERRAIN_CHUNK_COUNT][TERRAIN_CHUNK_COUNT]
                                   [TERRAIN_CHUNK_COUNT];

  public:
    VisualTerrain(Terrain* terrain, ID3D11Device* device);
    ~VisualTerrain();

    // Update the visual terrain's meshes by pulling the terrain's data
    void updateTerrainMeshes();

    // Return the current meshes for rendering.
    const std::vector<Mesh*>& getTerrainMeshes();
};

} // namespace Graphics
} // namespace Engine