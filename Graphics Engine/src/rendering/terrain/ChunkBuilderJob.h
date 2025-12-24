#pragma once

#include <mutex>

#include "math/Triangle.h"
#include "math/Vector3.h"

#include "datamodel/terrain/Terrain.h"
#include "rendering/resources/MeshBuilder.h"

constexpr int TERRAIN_SAMPLES_PER_CHUNK = 7;

namespace Engine {
using namespace Math;
using namespace Datamodel;

namespace Graphics {
// ChunkBuilderJob:
// Used to asynchronously generate the terrain mesh
struct ChunkBuilderJob {
    // The async thread will lock on this
    std::mutex async_lock;

    enum JobStatus { Inactive, Active, Done };
    JobStatus status;

    MeshBuilder builder;

    // Terrain Surface Data (Populated by Visual Terrain)
    Vector3 chunk_position; // Bottom-left Coordinates
    float chunk_size;
    unsigned int chunk_id;

    // +2 is to include border triangles, so normals across chunks (of the same
    // LOD) match
    float data[TERRAIN_SAMPLES_PER_CHUNK + 2][TERRAIN_SAMPLES_PER_CHUNK + 2]
              [TERRAIN_SAMPLES_PER_CHUNK + 2];

    std::unordered_map<Vector3, unsigned int> vertex_map;
    std::vector<Triangle> border_triangles;

  public:
    ChunkBuilderJob(MeshPool* terrain_pool);

    // Asynchronous execution by VisualTerrain.
    void buildChunkMesh();

  private:
    // Helpers used by the asynchronous function
    unsigned int loadVertexIntoBuilder(const Vector3& vertex);
};

} // namespace Graphics
} // namespace Engine