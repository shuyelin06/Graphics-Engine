#pragma once

#include <mutex>
#include <queue>
#include <vector>

#include "../Bindable.h"
#include "../Object.h"

#include "TerrainCallback.h"
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

enum BiomeType {};

enum BiomeSelector {};

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

    void seed(unsigned int seed);
    void forceInvalidateAll();

  private:
    void scheduleTerrainReload(const ChunkIndex& local_index,
                               const ChunkIndex& world_index);
    void reloadChunk(const ChunkIndex local_index,
                     const ChunkIndex world_index);
};

} // namespace Datamodel
} // namespace Engine
