#include "SceneManager.h"

#include <mutex>
#include <unordered_set>

#include "rendering/VisualSystem.h"
#include "rendering/resources/MaterialManager.h"
#include "rendering/resources/ResourceManager.h"

namespace Engine {
namespace Graphics {
using UpdatePacket = SceneManager::UpdatePacket;

struct DefaultMeshBlock {
    std::shared_ptr<Mesh> mesh = nullptr;
    std::shared_ptr<Material> material = nullptr;
    InstanceData instanceData{};

    DrawBlockKey blockKey = kInvalidDrawBlockKey;
};

class SceneManagerImpl {
  private:
    VisualSystem* mVisualSystem;

    std::vector<UpdatePacket> mUpdatePacketsScratch;
    std::mutex mUpdatePacketsLock;
    std::vector<UpdatePacket> mUpdatePackets;

    std::unordered_map<uint32_t, std::unique_ptr<Camera>> cameras;
    Camera* activeCamera = nullptr;

    std::unordered_map<uint32_t, DefaultMeshBlock> meshes;
    std::unordered_set<uint32_t> dirtyMeshes;

  public:
    SceneManagerImpl(VisualSystem* _visualSystem);
    ~SceneManagerImpl();

    void submitUpdatePacket(const UpdatePacket& packet);

    void update();

    Camera* getMainCamera();

  private:
    void processDirtyMeshes();
    void processUpdatePackets();

    void processCameraPacket(const UpdatePacket& packet);
    void processMeshPacket(const UpdatePacket& packet);
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
    processDirtyMeshes();
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

        if (std::holds_alternative<Camera::UpdatePacket>(packet.data))
            processCameraPacket(packet);
        else if (std::holds_alternative<RenderableMeshUpdatePacket>(
                     packet.data))
            processMeshPacket(packet);
        else
            assert(false); // Unimplemented
    }
}

void SceneManagerImpl::processDirtyMeshes() {
    RenderManager* renderManager = mVisualSystem->getRenderManager();

    std::unordered_set<uint32_t>::iterator iter;
    for (iter = dirtyMeshes.begin(); iter != dirtyMeshes.end();) {
        bool remove = false;

        if (!meshes.contains(*iter)) {
            remove = true;
        } else {
            auto& mesh = meshes[*iter];

            bool resourcesReady = true;
            if (mesh.mesh && !mesh.mesh->ready)
                resourcesReady = false;
            if (mesh.material && !mesh.material->ready())
                resourcesReady = false;

            if (resourcesReady) {
                if (mesh.mesh && mesh.material) {
                    assert(mesh.mesh->ready && mesh.material->ready());
                    assert(mesh.blockKey == kInvalidDrawBlockKey);

                    DrawBlock drawBlock;
                    drawBlock.initialize(AABB(), mesh.mesh.get(),
                                         mesh.material.get());
                    mesh.blockKey = renderManager->addDrawBlock(drawBlock);
                    renderManager->updateInstanceData(mesh.blockKey,
                                                      mesh.instanceData);
                }
                remove = true;
            }
        }

        if (remove) {
            iter = dirtyMeshes.erase(iter);
        } else {
            ++iter;
        }
    }
}

void SceneManagerImpl::processCameraPacket(const UpdatePacket& packet) {
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
}

void SceneManagerImpl::processMeshPacket(const UpdatePacket& packet) {
    RenderManager* renderManager = mVisualSystem->getRenderManager();

    switch (packet.operation) {
    case UpdatePacket::Operation::Create: {
        assert(!meshes.contains(packet.handle));
        meshes[packet.handle] = DefaultMeshBlock();
    } break;

    case UpdatePacket::Operation::Destroy: {
        assert(meshes.contains(packet.handle));
        auto& mesh = meshes[packet.handle];
        if (mesh.blockKey != kInvalidDrawBlockKey) {
            renderManager->removeDrawBlock(mesh.blockKey);
        }
        meshes.erase(packet.handle);
    } break;

    case UpdatePacket::Operation::Update: {
        assert(meshes.contains(packet.handle));
        auto& mesh = meshes[packet.handle];

        const RenderableMeshUpdatePacket& data =
            std::get<RenderableMeshUpdatePacket>(packet.data);
        bool isDirty = false;

        switch (data.type) {
        case RenderableMeshUpdatePacket::Property::LocalMatrix: {
            mesh.instanceData.mLocalToWorld = std::get<Matrix4>(data.data);
            mesh.instanceData.mNormalTransform =
                mesh.instanceData.mLocalToWorld.inverse().transpose();
            if (mesh.blockKey != kInvalidDrawBlockKey) {
                renderManager->updateInstanceData(mesh.blockKey,
                                                  mesh.instanceData);
            }
        } break;

        case RenderableMeshUpdatePacket::Property::MeshName: {
            const std::string& meshName = std::get<std::string>(data.data);
            mesh.mesh =
                mVisualSystem->getResourceManager()->LoadMeshFromFile(meshName);
            isDirty = true;
        } break;

        case RenderableMeshUpdatePacket::Property::ColorMapName: {
            const std::string& colormapName = std::get<std::string>(data.data);
            MaterialManager::DefaultMaterialParams params;
            params.colormap = colormapName;
            mesh.material =
                mVisualSystem->getMaterialManager()->createMaterial(params);
            isDirty = true;
        } break;

        default:
            assert(false);
            break;
        }

        if (isDirty) {
            dirtyMeshes.insert(packet.handle);
            if (mesh.blockKey != kInvalidDrawBlockKey) {
                renderManager->removeDrawBlock(mesh.blockKey);
                mesh.blockKey = kInvalidDrawBlockKey;
            }
        }
    } break;
    }
}

Camera* SceneManagerImpl::getMainCamera() { return activeCamera; }

} // namespace Graphics
} // namespace Engine