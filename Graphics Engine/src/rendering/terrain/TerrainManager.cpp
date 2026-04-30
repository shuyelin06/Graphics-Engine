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
#include "TerrainOctree.h"
#include "TerrainSDF.h"

constexpr int OCTREE_MAX_DEPTH = 4;

namespace Engine {
using namespace Datamodel;

namespace Graphics {
using UpdatePacket = TerrainManager::UpdatePacket;

struct MeshDescription {
    unsigned int index_start;
    unsigned int index_count;

    unsigned int vertex_start;
    unsigned int vertex_count;
};

class VisualTerrainImpl {
  private:
    VisualSystem* visual_system;

    bool enabled;
    std::unique_ptr<TerrainSDF> generator;
    std::unique_ptr<TerrainOctree> octree;

    DrawBlockKey drawBlockKey = kInvalidDrawBlockKey;

    StructuredBuffer sb_descriptors;
    StructuredBuffer sb_indices;
    StructuredBuffer sb_positions;
    StructuredBuffer sb_normals;
    int num_active_chunks;
    int max_chunk_triangles;
    std::unique_ptr<VertexTechnique> mMesh;
    std::unique_ptr<PixelTechnique> mPixelShader;

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
                                                const Vector3& camera_pos) {
    mImpl->updateAndUploadTerrainData(context, camera_pos);
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

    generator = TerrainSDF::create();
    octree = TerrainOctree::create(generator.get(),
                                   visual_system->getResourceManager());

    mMesh = std::make_unique<VertexTechnique>("Terrain");

    ID3D11Device* device = visual_system->getDevice();
    constexpr int MAX_CHUNK_COUNT =
        TERRAIN_CHUNK_COUNT * TERRAIN_CHUNK_COUNT * TERRAIN_CHUNK_COUNT;
    sb_descriptors.initialize(device, sizeof(MeshDescription), MAX_CHUNK_COUNT);
    sb_indices.initialize(device, sizeof(unsigned int), 200000 * 3);
    sb_positions.initialize(device, sizeof(Vector3), 300000);
    sb_normals.initialize(device, sizeof(Vector3), 300000);

    num_active_chunks = 0;
    max_chunk_triangles = 0;

    mPixelShader = std::make_unique<PixelTechnique>("Terrain");
}

VisualTerrainImpl::~VisualTerrainImpl() = default;

void VisualTerrainImpl::submitSceneUpdate(const UpdatePacket& packet) {
    std::scoped_lock<std::mutex> updatePacketsLock(mUpdatePacketsLock);
    mUpdatePacketsScratch.emplace_back(packet);
}

// GenerateTerrainMesh:
// Generates the mesh for the terrain
void VisualTerrainImpl::updateAndUploadTerrainData(ID3D11DeviceContext* context,
                                                   const Vector3& camera_pos) {
    processSceneUpdates();

    if (!enabled)
        return;

    ICPUTimer cpu_timer = CPUTimer::TrackCPUTime("Terrain Update");

    // Update the Octree
    octree->update(camera_pos);

    std::vector<Mesh*> terrain_meshes;
    octree->pullTerrainMeshes(terrain_meshes);

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
    std::vector<MeshDescription> descriptors;
    for (Mesh* mesh : terrain_meshes) {
        descriptors.emplace_back(mesh->triangle_start * 3,
                                 mesh->num_triangles * 3, mesh->vertex_start,
                                 mesh->num_vertices);
    }

    {
        num_active_chunks = descriptors.size();
        max_chunk_triangles = 0;

        for (const auto& descriptor : descriptors) {
            max_chunk_triangles =
                max(max_chunk_triangles, descriptor.index_count / 3);
        }

        sb_descriptors.uploadData(context, descriptors.data(),
                                  descriptors.size());
        sb_indices.uploadData(context, mesh_pool->cpu_ibuffer.get(),
                              mesh_pool->triangle_size * 3);
        sb_positions.uploadData(context,
                                mesh_pool->cpu_vbuffers[POSITION].get(),
                                mesh_pool->vertex_size);
        sb_normals.uploadData(context, mesh_pool->cpu_vbuffers[NORMAL].get(),
                              mesh_pool->vertex_size);

        mMesh->setVertexData(nullptr, max_chunk_triangles * 3,
                             num_active_chunks);
        mMesh->setStructuredBuffer(0, &sb_descriptors);
        mMesh->setStructuredBuffer(1, &sb_indices);
        mMesh->setStructuredBuffer(2, &sb_positions);
        mMesh->setStructuredBuffer(3, &sb_normals);
    }

    if (drawBlockKey == kInvalidDrawBlockKey) {
        RenderPassSet passes{};
        passes.addPass(RenderPass::kOpaque);

        DrawBlock drawBlock;
        drawBlock.initialize(AABB(), passes,
                             {mMesh.get(), mPixelShader.get()});
        drawBlockKey =
            visual_system->getRenderManager()->addDrawBlock(drawBlock);
    }
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
            octree = TerrainOctree::create(generator.get(),
                                           visual_system->getResourceManager());
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