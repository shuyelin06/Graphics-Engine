#pragma once

#include <vector>

#include "math/PerlinNoise.h"

#include "Object.h"
#include "terrain/Terrain.h"

// Stores the number of terrain chunks past the center chunk we will maintain.
/*
               /\ Extent
                |
                |
              -----
             |     |
  Extent<--  |     | --> Extent
              -----
                |
                |
               \/ Extent

*/
constexpr int TERRAIN_CHUNK_EXTENT = 3;
constexpr int TERRAIN_NUM_CHUNKS = 2 * TERRAIN_CHUNK_EXTENT + 1;

namespace Engine {
using namespace Math;

namespace Datamodel {
// Class SceneGraph:
// Stores and manages all objects in the scene. Objects are stored in a
// tree-like hierarchy, Parent <--> Children Where all children node transforms
// are based off the local coordinate system of their parent. The reference
// coordinate system of all nodes without a parent is the world coordinate
// system.
class Scene {
  private:
    std::vector<Object*> objects;

    // Random Generation
    PerlinNoise* noise_func;

    // --- Terrain Fields ---
    // Center chunk of the scene. Loading is performed based on this center
    int center_chunk_x, center_chunk_z;

    // Loaded terrain chunks in the scene
    TerrainChunk* terrain[TERRAIN_NUM_CHUNKS][TERRAIN_NUM_CHUNKS];
    // Temporarily stores terrain chunks for when the scene center changes
    TerrainChunk* terrain_helper[TERRAIN_NUM_CHUNKS][TERRAIN_NUM_CHUNKS];
    // Stores newly created chunks to be bound
    std::vector<TerrainChunk*> new_chunks;

  public:
    Scene();
    ~Scene();

    // --- Object Handling ---
    const std::vector<Object*>& getObjects();
    Object& createObject();

    // Update object transforms and submit render requests
    void updateObjects();

    // --- Terrain Handling ---
    TerrainChunk* getTerrainChunk(int x_index, int z_index) const;
    float sampleTerrainHeight(float x, float z) const;

    void seedTerrain(unsigned int seed); // Set the Generation Seed
    void reloadTerrainChunk(int x_index, int z_index); // Reload a Terrain Chunk
    void reloadTerrain();                              // Reload all terrain

    // Update the loaded terrain chunks based on the center x,z coordinates
    void updateTerrainChunks(float center_x, float center_z);
    // Returns the newly created terrain chunks
    const std::vector<TerrainChunk*>& getNewChunks() const;

  private:
    void updateObjectsHelper(Object* object, const Matrix4& m_parent);
};

} // namespace Datamodel
} // namespace Engine