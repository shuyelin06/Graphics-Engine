#pragma once

#include <array>
#include <queue>
#include <unordered_map>

#include "datamodel/terrain/Terrain.h"

#include "rendering/RenderPass.h"
#include "rendering/core/Frustum.h"
#include "rendering/core/Mesh.h"

#include "rendering/pipeline/StructuredBuffer.h"

#include "rendering/resources/MeshBuilder.h"
#include "rendering/resources/ResourceManager.h"

#include "ChunkBuilderJob.h"
#include "WaterSurface.h"

#include "Octree.h"

namespace Engine {
using namespace Datamodel;

namespace Graphics {
// Data Structures for the terrain structured buffers. These buffers
// will be used in the vertex shader to generate the terrain mesh using vertex
// pulling
struct TBChunkDescriptor {
    unsigned int index_start;
    unsigned int index_count;

    unsigned int vertex_start;
    unsigned int vertex_count;
};

// VisualTerrain Class:
// Interfaces with the datamodel to pull and generate terrain data for the
// visual system. This primarily consists of mesh data. To reduce the number of
// draw calls, VisualTerrain will dynamically group chunk meshes into a single
// vertex / index buffer.
class VisualTerrain {
  private:
    Terrain* terrain;

    // Water Surface
    WaterSurface* water_surface;
    float surface_level;

    // Output Chunk Meshes
    MeshPool* mesh_pool;
    // Map of Octree Leaf --> Chunk Mesh
    std::unordered_map<OctreeNodeID, std::shared_ptr<Mesh>> terrain_meshes;

    // Terrain Octree
    std::unique_ptr<Octree> octree;

    // Jobs
    std::vector<std::unique_ptr<ChunkBuilderJob>> jobs;
    std::vector<int> inactive_jobs;

    double total_time_taken;
    int total_finished_jobs;

    // Dirty (& unprocessed) chunks
    struct DirtyChunk {
        unsigned int chunk_id;
        float priority;
        bool operator<(const DirtyChunk& other) const {
            return priority < other.priority;
        }
    };
    std::priority_queue<DirtyChunk> dirty_chunks;

    // Config (set with ImGui)
    // Some Observations:
    // - LOD 0 Distance >> Voxel Size. Otherwise, you'll be able to see nodes
    //   update close to the camera which looks weird.
    struct {
        int octree_max_depth = 8;
        float voxel_size = 25.f;
    } config;

  public:
    VisualTerrain(Terrain* terrain, ID3D11DeviceContext* context,
                  ResourceManager& resource_manager);
    ~VisualTerrain();

    // Update the octree and pull the most recent terrain meshesS
    void updateAndUploadTerrainData(ID3D11DeviceContext* context,
                                    RenderPassTerrain& pass_terrain,
                                    const Vector3& camera_pos);

    // Return water surface data
    const WaterSurface* getWaterSurface() const;
    float getSurfaceLevel() const;

    // ImGui
    void imGui();

  private:
    void loadChunkJobData(ChunkBuilderJob& job,
                          const TerrainGenerator& generator,
                          const OctreeNode& chunk);
    float computeChunkPriority(const OctreeNode& chunk);
};

} // namespace Graphics
} // namespace Engine