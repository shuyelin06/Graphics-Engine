#pragma once

#include <vector>

#include "datamodel/bvh/BVH.h"
#include "datamodel/bvh/TLAS.h"

#include "math/Triangle.h"

#include "math/Vector2.h"
#include "math/Vector3.h"

#include "math/PerlinNoise.h"

// --- EDITABLE PARAMETERS ---
// Chunk Size
constexpr float TERRAIN_CHUNK_SIZE = 50.f;
// Samples Per Chunk (must be >= 2)
// Chunk will share the data values along their borders,
// so a higher sample count will mean less memory is wasted.
constexpr int TERRAIN_CHUNK_SAMPLES = 5;
// # Chunks out that will be loaded at once.
constexpr int TERRAIN_CHUNK_EXTENT = 9;
// ---

// # Chunks in 1 dimension
constexpr int TERRAIN_CHUNK_COUNT = 2 * TERRAIN_CHUNK_EXTENT + 1;

namespace Engine {
using namespace Math;

namespace Datamodel {
typedef unsigned int UINT;

// Terrain Class:
// Represents the terrain in a scene. Internally achieves this by
// storing terrain chunks as 3D grids of data, where the surface exists
// where 0 is (when interpolating the data across space).
struct Chunk {
    // Stores the chunk's data in its 8 corners.
    // These are given in order 000, 100, 110, 010, 001, 101, 111, 011 (xyz)
    float data[TERRAIN_CHUNK_SAMPLES][TERRAIN_CHUNK_SAMPLES]
              [TERRAIN_CHUNK_SAMPLES];

    // Stores what triangles in the triangle_pool belong to this chunk
    UINT triangle_start, triangle_count;
};

class Terrain {
  private:
    // Perlin Noise Generator
    PerlinNoise noise_func;

    // "Pointers" (indices) to the chunk pool, storing the active chunks
    // (centered around some position in space).
    int center_x, center_y, center_z; // Chunk Index Coordinates
    Chunk* chunks[TERRAIN_CHUNK_COUNT][TERRAIN_CHUNK_COUNT]
                 [TERRAIN_CHUNK_COUNT];
    // Temporary array used when the chunks are being updated
    Chunk* chunks_helper[TERRAIN_CHUNK_COUNT][TERRAIN_CHUNK_COUNT]
                        [TERRAIN_CHUNK_COUNT];

    // Chunk BVHs + TLAS for Raycasting
    TLAS tlas;

    BVH* bvh_array[TERRAIN_CHUNK_COUNT][TERRAIN_CHUNK_COUNT]
                 [TERRAIN_CHUNK_COUNT];
    BVH* bvh_helper[TERRAIN_CHUNK_COUNT][TERRAIN_CHUNK_COUNT]
                  [TERRAIN_CHUNK_COUNT];

    // Triangle Pool
    // All triangles owned by the terrain's chunks
    std::vector<Triangle> triangle_pool;
    std::vector<Triangle> triangle_pool_helper;

  public:
    Terrain();
    ~Terrain();

    // Accessors
    const TLAS& getTLAS() const;
    const std::vector<Triangle>& getTrianglePool() const;

    int getCenterChunkX() const;
    int getCenterChunkY() const;
    int getCenterChunkZ() const;

    const Chunk* getChunk(int x_i, int y_i, int z_i) const;

    // Reload terrain chunks based on a new center (x,y,z) in\
    // world coordinates.
    void reloadTerrain(float x, float y, float z);

  private:
    void loadChunk(int index_x, int index_y, int index_z, bool direct_load);
    void unloadChunk(int index_x, int index_y, int index_z);
};

} // namespace Datamodel
} // namespace Engine
