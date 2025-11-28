#pragma once

#include <mutex>

#include "math/Triangle.h"
#include "math/Vector3.h"

#include "datamodel/terrain/Terrain.h"
#include "rendering/resources/MeshBuilder.h"

namespace Engine {
using namespace Math;
using namespace Datamodel;

namespace Graphics {
// ChunkBuilderJob:
// Used to asynchronously generate the terrain mesh
struct ChunkBuilderJob {
    // The async thread will lock on this
    std::mutex async_lock;
    bool active;

    MeshBuilder builder;

    TerrainChunk chunk_copy;
    ChunkIndex chunk_array_index;

    std::unordered_map<Vector3, unsigned int> vertex_map;
    std::vector<Triangle> border_triangles;

  public:
    ChunkBuilderJob(MeshPool* terrain_pool);

    // Called by main thread to load data before execution
    void loadChunkData(const TerrainChunk& data, const ChunkIndex& arr_index);

    // Asynchronous execution by VisualTerrain.
    void buildChunkMesh();

  private:
    // Helpers used by the asynchronous function
    unsigned int loadVertexIntoBuilder(const Vector3& vertex);
};

} // namespace Graphics
} // namespace Engine