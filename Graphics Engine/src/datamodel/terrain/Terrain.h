#pragma once

#include <atomic>
#include <queue>
#include <vector>

#include "datamodel/bvh/BVH.h"
#include "datamodel/bvh/TLAS.h"

#include "math/Triangle.h"

#include "math/Vector2.h"
#include "math/Vector3.h"

#include "math/PerlinNoise.h"

// --- EDITABLE PARAMETERS ---
// Chunk Size
constexpr float TERRAIN_CHUNK_SIZE = 75.f;
// Samples Per Chunk (must be >= 2)
// Chunk will share the data values along their borders,
// so a higher sample count will mean less memory is wasted.
constexpr int TERRAIN_CHUNK_SAMPLES = 7;
// # Chunks out that will be loaded at once.
constexpr int TERRAIN_CHUNK_EXTENT = 6;
// ---

// # Chunks in 1 dimension
constexpr int TERRAIN_CHUNK_COUNT = 2 * TERRAIN_CHUNK_EXTENT + 1;

namespace Engine {
using namespace Math;

namespace Datamodel {
typedef unsigned int UINT;

// CHUNKS SHOULD NOT BE TOUCHED WHILE DIRTY == TRUE. THIS IS
// NOT SAFE.

// Terrain Class:
// Represents the terrain in a scene. Internally achieves this by
// storing terrain chunks as 3D grids of data, where the surface exists
// where 0 is (when interpolating the data across space).
struct Chunk {
    // Tracks if the chunk is dirty or not. If the chunk
    // is dirty, it needs to be reloaded.
    std::atomic<bool> dirty;

    // Stores the chunk's x,y,z world index
    int chunk_x, chunk_y, chunk_z;
    // Stores the chunk's data in its 8 corners.
    float data[TERRAIN_CHUNK_SAMPLES][TERRAIN_CHUNK_SAMPLES]
              [TERRAIN_CHUNK_SAMPLES];
    // Stores what triangles in the triangle_pool belong to this chunk
    std::vector<Triangle> triangles;

  public:
    void setDirty(bool status);
    bool isDirty() const;
};

class Terrain {
  private:
    // Perlin Noise Generator
    PerlinNoise noise_func;

    // "Pointers" (indices) to the chunk pool, storing the active chunks
    // (centered around some position in space).
    int center_x, center_y, center_z; // Chunk Index Coordinates
    Chunk chunks[TERRAIN_CHUNK_COUNT][TERRAIN_CHUNK_COUNT][TERRAIN_CHUNK_COUNT];

    // Chunk BVHs + TLAS for Raycasting
    TLAS tlas;
    BVH bvh_array[TERRAIN_CHUNK_COUNT][TERRAIN_CHUNK_COUNT]
                 [TERRAIN_CHUNK_COUNT];

  public:
    Terrain();
    ~Terrain();

    // Accessors
    const TLAS& getTLAS() const;

    int getCenterChunkX() const;
    int getCenterChunkY() const;
    int getCenterChunkZ() const;

    const Chunk* getChunk(int x_i, int y_i, int z_i) const;

    // Invalidate terrain chunks based on a new center (x,y,z) in
    // world coordinates. Submits generation requests to a worker
    // thread
    void invalidateTerrain(float x, float y, float z);
    // Check results of worker threads and load them into the terrain
    // when ready.

  private:
    void scheduleTerrainReload(int index_x, int index_y, int index_z);
    void loadChunk(int index_x, int index_y, int index_z);
    void unloadChunk(int index_x, int index_y, int index_z);
};

} // namespace Datamodel
} // namespace Engine
