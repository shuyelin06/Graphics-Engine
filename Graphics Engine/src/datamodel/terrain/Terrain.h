#pragma once

#include <mutex>
#include <queue>
#include <vector>

#include "TerrainCallback.h"
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
// Must be >= 2, since we must at least sample the borders.
constexpr int TERRAIN_CHUNK_SAMPLES = 7;

// # Chunks out that will be loaded at once.
constexpr int TERRAIN_CHUNK_EXTENT = 7;

// Water Line
constexpr float TERRAIN_FADE_LINE = 100.f;
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
struct ChunkIndex {
    int x, y, z;
};

struct TerrainChunk {
    // Mutex for the chunk.
    std::mutex mutex;

    // Stores the chunk's x,y,z world index
    int chunk_x, chunk_y, chunk_z;
    // Stores the chunk's data in its 8 corners. Includes a few samples
    // outside of the chunk so that we can smooth out the normals.
    float data[TERRAIN_CHUNK_SAMPLES + 2][TERRAIN_CHUNK_SAMPLES + 2]
              [TERRAIN_CHUNK_SAMPLES + 2];
    // Stores what triangles in the triangle_pool belong to this chunk
    std::vector<Triangle> triangles;
    // Stores the border triangles. These contribute to the normals but
    // will not be in the final mesh.
    std::vector<Triangle> border_triangles;

    // Terrain callbacks, which are called on chunk reload. Systems can use
    // these callbacks to regenerate terrain data.
    std::vector<TerrainCallback*> callbacks;
};

class Terrain {
  private:
    // Perlin Noise Generator
    PerlinNoise noise_func;

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

    // --- Initialization ---
    void registerTerrainCallback(int i, int j, int k,
                                 TerrainCallback* callback);

    // --- Accessors ---
    float getSurfaceHeight() const;

    // --- Update Function ---
    // Invalidate terrain chunks based on a new center (x,y,z) in
    // world coordinates. Invalidated terrain chunks have generation requests
    // submitted to worker threads so that their data can be generated again.
    void invalidateTerrain(float x, float y, float z);

  private:
    void scheduleTerrainReload(const ChunkIndex& local_index,
                               const ChunkIndex& world_index);
    void reloadChunk(const ChunkIndex local_index,
                     const ChunkIndex world_index);
};

} // namespace Datamodel
} // namespace Engine
