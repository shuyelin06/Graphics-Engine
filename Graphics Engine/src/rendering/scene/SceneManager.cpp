#include "SceneManager.h"

#include <mutex>

#include "rendering/VisualSystem.h"

#include "rendering/pipeline/techniques/VSPSMesh.h"

namespace Engine {
namespace Graphics {
using UpdatePacket = SceneManager::UpdatePacket;

struct RenderableMeshBlock {
    std::unique_ptr<RenderableMesh> mesh;
    DrawBlockKey blockKey = kInvalidDrawBlockKey;

    std::unique_ptr<VSMesh> vsMesh;
    std::unique_ptr<PSMesh> psMesh;
};

class SceneManagerImpl {
  private:
    VisualSystem* mVisualSystem;

    std::vector<UpdatePacket> mUpdatePacketsScratch;
    std::mutex mUpdatePacketsLock;
    std::vector<UpdatePacket> mUpdatePackets;

    std::unordered_map<uint32_t, std::unique_ptr<Camera>> cameras;
    Camera* activeCamera = nullptr;

    std::unordered_map<uint32_t, RenderableMeshBlock> meshes;
    std::vector<std::pair<VSMesh, PSMesh>> meshTechniques;

  public:
    SceneManagerImpl(VisualSystem* _visualSystem);
    ~SceneManagerImpl();

    void submitUpdatePacket(const UpdatePacket& packet);

    void update();

    Camera* getMainCamera();

  private:
    void processUpdatePackets();
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

void SceneManagerImpl::update() { processUpdatePackets(); }

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
                meshes[packet.handle] = {
                    std::make_unique<RenderableMesh>(
                        mVisualSystem->getResourceManager()),
                    kInvalidDrawBlockKey, std::make_unique<VSMesh>(),
                    std::make_unique<PSMesh>(
                        mVisualSystem->getResourceManager())};

                auto& meshBlock = meshes[packet.handle];

                RenderPassSet passes{};
                passes.addPass(RenderPass::kOpaque);
                DrawBlock drawBlock;
                drawBlock.initialize(
                    AABB(), passes,
                    {meshBlock.vsMesh.get(), meshBlock.psMesh.get()});
                meshBlock.blockKey =
                    mVisualSystem->getRenderManager()->addDrawBlock(drawBlock);
            } break;

            case UpdatePacket::Operation::Destroy: {
                assert(meshes.contains(packet.handle));
                auto& meshBlock = meshes[packet.handle];
                mVisualSystem->getRenderManager()->removeDrawBlock(
                    meshBlock.blockKey);
                meshes.erase(packet.handle);
            } break;

            case UpdatePacket::Operation::Update: {
                assert(meshes.contains(packet.handle));
                auto& meshBlock = meshes[packet.handle];
                meshBlock.mesh->update(
                    std::get<RenderableMesh::UpdatePacket>(packet.data));
                meshBlock.vsMesh->update(meshBlock.mesh->getMesh(),
                                         meshBlock.mesh->getLocalMatrix());
                meshBlock.psMesh->update(meshBlock.mesh->getMaterial());
            } break;
            }
        } else
            assert(false);
    }
}

Camera* SceneManagerImpl::getMainCamera() { return activeCamera; }

} // namespace Graphics
} // namespace Engine