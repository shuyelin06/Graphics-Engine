#include "SceneManager.h"

#include <mutex>

#include "rendering/VisualSystem.h"

#include "rendering/pipeline/techniques/VSPSMesh.h"

namespace Engine {
namespace Graphics {
using UpdatePacket = SceneManager::UpdatePacket;
class SceneManagerImpl {
  private:
    VisualSystem* mVisualSystem;

    std::vector<UpdatePacket> mUpdatePacketsScratch;
    std::mutex mUpdatePacketsLock;
    std::vector<UpdatePacket> mUpdatePackets;

    std::unordered_map<uint32_t, std::unique_ptr<Camera>> cameras;
    Camera* activeCamera = nullptr;

    std::unordered_map<uint32_t, std::unique_ptr<RenderableMesh>> meshes;
    std::vector<std::pair<VSMesh, PSMesh>> meshTechniques;

  public:
    SceneManagerImpl(VisualSystem* _visualSystem);
    ~SceneManagerImpl();

    void submitUpdatePacket(const UpdatePacket& packet);

    void update();

    Camera* getMainCamera();

  private:
    void processUpdatePackets();
    void submitDrawCalls();
};

SceneManager::SceneManager() {}
SceneManager::~SceneManager() = default;

void SceneManager::submitUpdatePacket(const UpdatePacket& packet) {
    mImpl->submitUpdatePacket(packet);
}

void SceneManager::update() { mImpl->update(); }

Camera* SceneManager::getMainCamera() { return mImpl->getMainCamera(); }

std::unique_ptr<SceneManager> SceneManager::create(VisualSystem* visualSystem) {
    std::unique_ptr<SceneManager> ptr =
        std::unique_ptr<SceneManager>(new SceneManager());
    ptr->mImpl = std::make_unique<SceneManagerImpl>(visualSystem);
    return ptr;
}

SceneManagerImpl::SceneManagerImpl(VisualSystem* _visualSystem)
    : mVisualSystem(_visualSystem) {}
SceneManagerImpl::~SceneManagerImpl() = default;

void SceneManagerImpl::submitUpdatePacket(const UpdatePacket& packet) {
    std::scoped_lock<std::mutex> updatePacketsLock(mUpdatePacketsLock);
    mUpdatePacketsScratch.emplace_back(packet);
}

void SceneManagerImpl::update() {
    processUpdatePackets();
    submitDrawCalls();
}

void SceneManagerImpl::processUpdatePackets() {
    {
        std::scoped_lock<std::mutex> updatePacketsLock(mUpdatePacketsLock);
        std::swap(mUpdatePackets, mUpdatePacketsScratch);
        mUpdatePacketsScratch.clear();
    }

    while (!mUpdatePackets.empty()) {
        const UpdatePacket packet = mUpdatePackets.back();
        mUpdatePackets.pop_back();

        if (std::holds_alternative<Camera::UpdatePacket>(packet.data)) {
            switch (packet.operation) {
            case UpdatePacket::Operation::Create: {
                assert(!cameras.contains(packet.handle));
                cameras[packet.handle] = std::make_unique<Camera>();
                // HACK
                activeCamera = cameras[packet.handle].get();
            } break;

            case UpdatePacket::Operation::Destroy: {
                assert(cameras.contains(packet.handle));
                cameras.erase(packet.handle);
            } break;

            case UpdatePacket::Operation::Update: {
                assert(cameras.contains(packet.handle));
                cameras[packet.handle]->update(
                    std::get<Camera::UpdatePacket>(packet.data));
            } break;
            }
        } else if (std::holds_alternative<RenderableMesh::UpdatePacket>(
                       packet.data)) {
            switch (packet.operation) {
            case UpdatePacket::Operation::Create: {
                assert(!meshes.contains(packet.handle));
                meshes[packet.handle] = std::make_unique<RenderableMesh>(
                    mVisualSystem->getResourceManager());
            } break;

            case UpdatePacket::Operation::Destroy: {
                assert(meshes.contains(packet.handle));
                meshes.erase(packet.handle);
            } break;

            case UpdatePacket::Operation::Update: {
                assert(meshes.contains(packet.handle));
                meshes[packet.handle]->update(
                    std::get<RenderableMesh::UpdatePacket>(packet.data));
            } break;
            }
        } else
            assert(false);
    }
}

void SceneManagerImpl::submitDrawCalls() {
    meshTechniques.clear();
    for (const auto& pair : meshes) {
        VSMesh vsMesh = VSMesh();
        PSMesh psMesh = PSMesh(mVisualSystem->getResourceManager());

        vsMesh.update(pair.second->getMesh(), pair.second->getLocalMatrix());
        psMesh.update(pair.second->getMaterial());

        meshTechniques.push_back({vsMesh, psMesh});

        RenderPassSet passes{};
        passes.addPass(RenderPass::kOpaque);

        mVisualSystem->getRenderManager()->submitDrawCall(
            {&meshTechniques.back().first, &meshTechniques.back().second},
            passes);
    }
}

Camera* SceneManagerImpl::getMainCamera() { return activeCamera; }

} // namespace Graphics
} // namespace Engine