#pragma once

#include <climits>
#include <mutex>
#include <queue>
#include <vector>

#include "../Bindable.h"
#include "../Object.h"

#include "TerrainConfig.h"
#include "datamodel/bvh/BVH.h"
#include "datamodel/bvh/TLAS.h"

#include "math/Triangle.h"

#include "math/Vector2.h"
#include "math/Vector3.h"

#include "math/PerlinNoise.h"

namespace Engine {
using namespace Math;

namespace Datamodel {
typedef unsigned int UINT;

// Terrain Class:
// Represents the terrain in a scene. Internally achieves this by
// storing terrain chunks as 3D grids of data, where the surface exists
// where 0 is (when interpolating the data across space).
struct ChunkIndex {
    int x, y, z;
};

struct TerrainChunk {
    // Stores the chunk's x,y,z world index
    int chunk_x = INT_MAX;
    int chunk_y = INT_MAX;
    int chunk_z = INT_MAX;

    // Stores the chunk's data in its 8 corners. Includes a few samples
    // outside of the chunk so that we can smooth out the normals.
    float data[TERRAIN_CHUNK_SAMPLES + 2][TERRAIN_CHUNK_SAMPLES + 2]
              [TERRAIN_CHUNK_SAMPLES + 2];

    // Chunk Update ID. Incremented whenever chunk is written to, so other
    // systems know when the chunk has been updated.
    int update_id = 0;
};

class Terrain : public Object, public Bindable<Terrain> {
  private:
    // Perlin Noise Generator
    PerlinNoise noise_func;
    unsigned int cur_seed;

    // Water "surface" height
    float surface_height;

    // "Pointers" (indices) to the chunk pool, storing the active chunks
    // (centered around some position in space).
    int center_x, center_y, center_z; // Chunk Index Coordinates
    TerrainChunk chunks[TERRAIN_CHUNK_COUNT][TERRAIN_CHUNK_COUNT]
                       [TERRAIN_CHUNK_COUNT];

  public:
    Terrain();
    ~Terrain();

    void propertyDisplay() override;

    // --- Accessors ---
    float getSurfaceHeight() const;

    const TerrainChunk& getChunk(int i, int j, int k);
    const TerrainChunk& getChunk(const ChunkIndex& arr_index);

    // --- Update Function ---
    // Invalidate terrain chunks based on a new center (x,y,z) in
    // world coordinates. Invalidated terrain chunks have generation requests
    // submitted to worker threads so that their data can be generated again.
    void invalidateTerrain(float x, float y, float z);
    void seed(unsigned int seed);

  private:
    void checkAndReloadChunks(bool force_invalidate = false);
    void reloadChunk(TerrainChunk* chunk, const ChunkIndex& world_index);
};

} // namespace Datamodel
} // namespace Engine
