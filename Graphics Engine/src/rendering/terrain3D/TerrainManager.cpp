#include "TerrainManager.h"

#include <assert.h>
#include <unordered_map>

#include "core/ThreadPool.h"

#include "math/Vector3.h"

#include "rendering/ImGui.h"
#include "rendering/util/CPUTimer.h"

#include "rendering/VisualSystem.h"

#include "rendering/core/Mesh.h"
#include "rendering/resources/MeshBuilder.h"

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
    octree = TerrainOctree::create(
        generator.get(), visual_system->getResourceManager(),
        visual_system->getMaterialManager(), visual_system->getRenderManager());
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

    octree->updateMeshLODs(camera_pos);
    octree->serveBuildRequests();
    octree->updateDrawBlocks();
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
                                           visual_system->getResourceManager(),
                                           visual_system->getMaterialManager(),
                                           visual_system->getRenderManager());
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