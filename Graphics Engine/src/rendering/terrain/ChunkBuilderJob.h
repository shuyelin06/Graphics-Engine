#pragma once

#include <mutex>

#include "math/Triangle.h"
#include "math/Vector3.h"

#include "utility/Stopwatch.h"

#include "datamodel/terrain/Terrain.h"
#include "rendering/resources/MeshBuilder.h"

constexpr int TERRAIN_SAMPLES_PER_CHUNK = 7;

namespace Engine {
using namespace Math;
using namespace Utility;
using namespace Datamodel;

namespace Graphics {
// ChunkBuilderJob:
// Used to asynchronously generate the terrain mesh
struct ChunkBuilderJob {
    // Synchronization with VisualTerrain
    std::mutex async_lock;

    enum JobStatus { Inactive, Active, Done };
    JobStatus status;

    // Terrain Surface Data (Populated by Visual Terrain)
    // +2 in data is to include border triangles, so normals across chunks (of
    // the same LOD) match
    Vector3 chunk_position; // Bottom-left Coordinates
    float chunk_size;
    unsigned int chunk_id;

    float data[TERRAIN_SAMPLES_PER_CHUNK + 2][TERRAIN_SAMPLES_PER_CHUNK + 2]
              [TERRAIN_SAMPLES_PER_CHUNK + 2];

    // Mesh Building
    MeshBuilder builder;
    std::unordered_map<Vector3, unsigned int> vertex_map;
    std::vector<Triangle> border_triangles;

    // Stats (Execution Time)
    Stopwatch stopwatch;
    double time_taken;

  public:
    ChunkBuilderJob(MeshPool* terrain_pool);

    // Asynchronous execution by VisualTerrain.
    void buildChunkMesh();

  private:
    // Helpers used by the asynchronous function
    void loadQuadIntoBuilder(const Vector3& a, const Vector3& b,
                             const Vector3& c, const Vector3& d);
    void loadTriangleIntoBuilder(const Triangle& triangle);
    unsigned int loadVertexIntoBuilder(const Vector3& vertex);
};

} // namespace Graphics
} // namespace Engine