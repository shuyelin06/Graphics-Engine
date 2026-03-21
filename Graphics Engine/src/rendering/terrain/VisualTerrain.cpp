#include "VisualTerrain.h"

#include <assert.h>
#include <unordered_map>

#include "core/ThreadPool.h"

#include "math/AABB.h"
#include "math/Vector3.h"

#include "rendering/ImGui.h"

#include "../VisualDebug.h"
#include "../util/CPUTimer.h"

#include "rendering/VisualSystem.h"

#include "rendering/core/Frustum.h"
#include "rendering/core/Mesh.h"
#include "rendering/pipeline/StructuredBuffer.h"
#include "rendering/resources/MeshBuilder.h"
#include "rendering/resources/ResourceManager.h"

#include "ChunkBuilderJob.h"
#include "Octree.h"
#include "TerrainGeneration.h"

constexpr int OCTREE_MAX_DEPTH = 4;

namespace Engine {
using namespace Datamodel;

namespace Graphics {
using UpdatePacket = VisualTerrain::UpdatePacket;

// Data Structures for the terrain structured buffers. These buffers
// will be used in the vertex shader to generate the terrain mesh using vertex
// pulling
struct TBChunkDescriptor {
    unsigned int index_start;
    unsigned int index_count;

    unsigned int vertex_start;
    unsigned int vertex_count;
};

class VisualTerrainImpl {
  private:
    VisualSystem* visual_system;

    bool enabled;
    std::unique_ptr<TerrainGeneration> generator;

    // Scene Update Queue
    std::vector<UpdatePacket> mUpdatePacketsScratch;
    std::mutex mUpdatePacketsLock;
    std::vector<UpdatePacket> mUpdatePackets;

    // Water Surface
    WaterSurface* water_surface;
    float surface_level;

    // Terrain Octree
    std::unique_ptr<TerrainMeshLoader> octree;

    // Config (set with ImGui)
    // Some Observations:
    // - LOD 0 Distance >> Voxel Size. Otherwise, you'll be able to see nodes
    //   update close to the camera which looks weird.
    struct {
        int octree_max_depth = 8;
        float voxel_size = 25.f;
    } config;

  public:
    VisualTerrainImpl(VisualSystem* visualSystem);
    ~VisualTerrainImpl();

    // Scene Updating
    void submitSceneUpdate(const UpdatePacket& packet);

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
    void processSceneUpdates();

    void loadChunkJobData(ChunkBuilderJob& job,
                          const TerrainGeneration& generator,
                          const TerrainNode& chunk);
    float computeChunkPriority(const TerrainNode& chunk);
};

VisualTerrain::VisualTerrain() = default;
VisualTerrain::~VisualTerrain() = default;

std::unique_ptr<VisualTerrain>
VisualTerrain::create(VisualSystem* visualSystem) {
    std::unique_ptr<VisualTerrain> ptr =
        std::unique_ptr<VisualTerrain>(new VisualTerrain());
    ptr->mImpl = std::make_unique<VisualTerrainImpl>(visualSystem);
    return std::move(ptr);
}

void VisualTerrain::submitSceneUpdate(const UpdatePacket& packet) {
    mImpl->submitSceneUpdate(packet);
}
void VisualTerrain::updateAndUploadTerrainData(ID3D11DeviceContext* context,
                                               RenderPassTerrain& pass_terrain,
                                               const Vector3& camera_pos) {
    mImpl->updateAndUploadTerrainData(context, pass_terrain, camera_pos);
}
const WaterSurface* VisualTerrain::getWaterSurface() const {
    return mImpl->getWaterSurface();
}
float VisualTerrain::getSurfaceLevel() const {
    return mImpl->getSurfaceLevel();
}
void VisualTerrain::imGui() { mImpl->imGui(); }

VisualTerrainImpl::VisualTerrainImpl(VisualSystem* m_visual_system) {
    visual_system = m_visual_system;

    enabled = false;
    generator = std::make_unique<TerrainGeneration>();

    // Initialize my water surface mesh.
    water_surface = new WaterSurface();
    water_surface->generateSurfaceMesh(visual_system->getResourceManager(), 15);
    water_surface->generateWaveConfig(14);

    surface_level = 0.f;

    octree = std::make_unique<TerrainMeshLoader>(config.octree_max_depth,
                                                 config.voxel_size);
}

VisualTerrainImpl::~VisualTerrainImpl() = default;

void VisualTerrainImpl::submitSceneUpdate(const UpdatePacket& packet) {
    std::scoped_lock<std::mutex> updatePacketsLock(mUpdatePacketsLock);
    mUpdatePacketsScratch.emplace_back(packet);
}

// GenerateTerrainMesh:
// Generates the mesh for the terrain
void VisualTerrainImpl::updateAndUploadTerrainData(
    ID3D11DeviceContext* context, RenderPassTerrain& pass_terrain,
    const Vector3& camera_pos) {
    processSceneUpdates();

    if (!enabled)
        return;

    ICPUTimer cpu_timer = CPUTimer::TrackCPUTime("Terrain Update");

    // 1) Update the Octree based on the camera
    // TODO: Move Updater out of here
    OctreeUpdater updater = octree->getUpdater();
    updater.updatePointOfFocus(camera_pos);
    float accumulated_size = config.voxel_size * 1.5f;
    for (int i = 0; i < config.octree_max_depth - 1; i++) {
        accumulated_size *= 2;
        // Guarantee valid input for now
        updater.updateLODDistance(i, accumulated_size);
    }

    octree->updateOctree(updater);
    octree->serveBuildRequests(generator.get(),
                               visual_system->getResourceManager());

    std::vector<Mesh*> terrain_meshes;
    octree->findValidMeshes(terrain_meshes);

    // Temp... Should be in ResourceManager.
    MeshPool* mesh_pool =
        visual_system->getResourceManager()->getMeshPool(MeshPoolType_Terrain);
    // TODO..
    mesh_pool->cleanAndCompact();

    // If anything wrote to our mesh pool, then we will upload the data to
    // GPU again.
    // TODO: We could throttle this so we only clean and compact every XX
    // frames..
    // Upload my data to the structured buffers
    std::vector<TBChunkDescriptor> descriptors;

    pass_terrain.num_active_chunks = terrain_meshes.size();
    pass_terrain.max_chunk_triangles = 0;
    for (Mesh* mesh : terrain_meshes) {
        TBChunkDescriptor desc;
        desc.index_start = mesh->triangle_start * 3;
        desc.index_count = mesh->num_triangles * 3;
        desc.vertex_start = mesh->vertex_start;
        desc.vertex_count = mesh->num_vertices;
        descriptors.push_back(desc);

        pass_terrain.max_chunk_triangles =
            max(pass_terrain.max_chunk_triangles, mesh->num_triangles);
    }

    pass_terrain.sb_chunks.uploadData(context, descriptors.data(),
                                      descriptors.size());
    pass_terrain.sb_indices.uploadData(context, mesh_pool->cpu_ibuffer.get(),
                                       mesh_pool->triangle_size * 3);
    pass_terrain.sb_positions.uploadData(
        context, mesh_pool->cpu_vbuffers[POSITION].get(),
        mesh_pool->vertex_size);
    pass_terrain.sb_normals.uploadData(
        context, mesh_pool->cpu_vbuffers[NORMAL].get(), mesh_pool->vertex_size);
}

void VisualTerrainImpl::processSceneUpdates() {
    {
        std::scoped_lock<std::mutex> updatePacketsLock(mUpdatePacketsLock);
        std::swap(mUpdatePackets, mUpdatePacketsScratch);
        mUpdatePacketsScratch.clear();
    }

    while (!mUpdatePackets.empty()) {
        const auto updatePacket = mUpdatePackets.back();
        mUpdatePackets.pop_back();

        switch (updatePacket.type) {
        case UpdatePacket::Type::kToggleTerrain:
            enabled = std::get<bool>(updatePacket.data);
            break;
        case UpdatePacket::Type::kPropertySeed: {
            const uint32_t seed = std::get<uint32_t>(updatePacket.data);
            generator->seedGenerator(seed);
            octree = std::make_unique<TerrainMeshLoader>(
                config.octree_max_depth, config.voxel_size);
        } break;
        }
    }
}

void VisualTerrainImpl::loadChunkJobData(ChunkBuilderJob& job,
                                         const TerrainGeneration& generator,
                                         const TerrainNode& chunk) {
    assert(chunk.isLeaf());

    job.vertex_map.clear();
    job.border_triangles.clear();

    job.builder.reset();
    job.builder.addLayout(POSITION);
    job.builder.addLayout(NORMAL);

    // Load Sampled Data
    job.chunk_id = chunk.uniqueID;
    job.chunk_position =
        chunk.center - Vector3(chunk.extents, chunk.extents, chunk.extents);
    job.chunk_size = chunk.extents * 2;

    const float DISTANCE_BETWEEN_SAMPLES =
        job.chunk_size / (TERRAIN_SAMPLES_PER_CHUNK - 1);

    for (int i = 0; i < TERRAIN_SAMPLES_PER_CHUNK + 2; i++) {
        for (int j = 0; j < TERRAIN_SAMPLES_PER_CHUNK + 2; j++) {
            for (int k = 0; k < TERRAIN_SAMPLES_PER_CHUNK + 2; k++) {
                const float sample_x =
                    job.chunk_position.x + (i - 1) * DISTANCE_BETWEEN_SAMPLES;
                const float sample_y =
                    job.chunk_position.y + (j - 1) * DISTANCE_BETWEEN_SAMPLES;
                const float sample_z =
                    job.chunk_position.z + (k - 1) * DISTANCE_BETWEEN_SAMPLES;

                job.data[i][j][k] = generator.sampleTerrainGenerator(
                    sample_x, sample_y, sample_z);
            }
        }
    }
}

float VisualTerrainImpl::computeChunkPriority(const TerrainNode& chunk) {
    float distance =
        (Vector3(chunk.center.x, chunk.center.y, chunk.center.z)).magnitude();
    return 1 / (1 + distance);
}

// --- Accessors ---
float VisualTerrainImpl::getSurfaceLevel() const { return surface_level; }

// Returns the water surface mesh
const WaterSurface* VisualTerrainImpl::getWaterSurface() const {
    return water_surface;
}

// ImGui
void VisualTerrainImpl::imGui() {
#if defined(IMGUI_ENABLED)
    ImGui::Text("Visual Terrain");

    if (octree && ImGui::CollapsingHeader("Octree")) {
        ImGui::Text("Octree Config");
        ImGui::Indent();
        {
            ImGui::SliderInt("Max Divisions", &config.octree_max_depth, 2, 14);
            ImGui::SliderFloat("Voxel Size", &config.voxel_size, 5.f, 50.f);

            if (ImGui::Button("Reset Octree")) {
                octree->resetOctree(config.octree_max_depth, config.voxel_size);
            }
        }
        ImGui::Unindent();

        ImGui::Text("Octree Display Settings");
        ImGui::Indent();
        {
            static bool display_octree = false;
            ImGui::Checkbox("Display Octree", &display_octree);

            if (display_octree) {
                static int target_lod = 0;
                ImGui::SliderInt("LOD", &target_lod, -1,
                                 config.octree_max_depth);

                const auto& node_map = octree->getNodeMap();
                for (const auto& pair : node_map) {
                    const auto& node = pair.second;

                    if (node->isLeaf() &&
                        (target_lod == -1 || node->depth == target_lod)) {
                        const float& extents = node->extents;
                        const Vector3 box_min =
                            node->center - Vector3(extents, extents, extents);
                        const Vector3 box_max =
                            node->center + Vector3(extents, extents, extents);

                        // TODO: Rename to DrawBox tbh
                        VisualDebug::DrawPoint(node->center, extents * 2);
                    }
                }
            }
        }
        ImGui::Unindent();
    }

#endif
}

} // namespace Graphics
} // namespace Engine