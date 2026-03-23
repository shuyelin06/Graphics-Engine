#include "TerrainManager.h"

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
#include "TerrainGeneration.h"
#include "TerrainOctree.h"

constexpr int OCTREE_MAX_DEPTH = 4;

namespace Engine {
using namespace Datamodel;

namespace Graphics {
using UpdatePacket = TerrainManager::UpdatePacket;

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
    std::unique_ptr<TerrainOctree> octree;

    // Scene Update Queue
    std::vector<UpdatePacket> mUpdatePacketsScratch;
    std::mutex mUpdatePacketsLock;
    std::vector<UpdatePacket> mUpdatePackets;

    // Water Surface
    WaterSurface* water_surface;
    float surface_level;

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
};

TerrainManager::TerrainManager() = default;
TerrainManager::~TerrainManager() = default;

std::unique_ptr<TerrainManager>
TerrainManager::create(VisualSystem* visualSystem) {
    std::unique_ptr<TerrainManager> ptr =
        std::unique_ptr<TerrainManager>(new TerrainManager());
    ptr->mImpl = std::make_unique<VisualTerrainImpl>(visualSystem);
    return std::move(ptr);
}

void TerrainManager::submitSceneUpdate(const UpdatePacket& packet) {
    mImpl->submitSceneUpdate(packet);
}
void TerrainManager::updateAndUploadTerrainData(ID3D11DeviceContext* context,
                                                RenderPassTerrain& pass_terrain,
                                                const Vector3& camera_pos) {
    mImpl->updateAndUploadTerrainData(context, pass_terrain, camera_pos);
}
const WaterSurface* TerrainManager::getWaterSurface() const {
    return mImpl->getWaterSurface();
}
float TerrainManager::getSurfaceLevel() const {
    return mImpl->getSurfaceLevel();
}
void TerrainManager::imGui() { mImpl->imGui(); }

VisualTerrainImpl::VisualTerrainImpl(VisualSystem* m_visual_system) {
    visual_system = m_visual_system;

    enabled = false;

    // Initialize my water surface mesh.
    water_surface = new WaterSurface();
    water_surface->generateSurfaceMesh(visual_system->getResourceManager(), 15);
    water_surface->generateWaveConfig(14);

    surface_level = 0.f;

    generator = TerrainGeneration::create();
    octree = TerrainOctree::create(generator.get());
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

    // Update the Octree
    octree->update(camera_pos);
    octree->serveBuildRequests(visual_system->getResourceManager());

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
            octree = TerrainOctree::create(generator.get());
        } break;
        }
    }
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
#endif
}

} // namespace Graphics
} // namespace Engine