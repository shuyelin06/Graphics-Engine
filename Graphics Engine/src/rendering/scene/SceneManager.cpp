#include "SceneManager.h"

#include <mutex>

#include "rendering/VisualSystem.h"

namespace Engine {
namespace Graphics {
using UpdatePacket = SceneManager::UpdatePacket;

struct DefaultMeshBlock {
    std::unique_ptr<DefaultMesh> mesh;

    DrawBlockKey blockKey = kInvalidDrawBlockKey;
    bool dirty = false;
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

  public:
    SceneManagerImpl(VisualSystem* _visualSystem);
    ~SceneManagerImpl();

    void submitUpdatePacket(const UpdatePacket& packet);

    void update();

    Camera* getMainCamera();

  private:
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

        if (std::holds_alternative<Camera::UpdatePacket>(packet.data))
            processCameraPacket(packet);
        else if (std::holds_alternative<DefaultMesh::UpdatePacket>(packet.data))
            processMeshPacket(packet);
        else
            assert(false); // Unimplemented
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

struct MeshTransform {
    Matrix4 localToWorld;
    Matrix4 normalTransform;
};

void SceneManagerImpl::processMeshPacket(const UpdatePacket& packet) {
    switch (packet.operation) {
    case UpdatePacket::Operation::Create: {
        assert(!meshes.contains(packet.handle));
        meshes[packet.handle] = DefaultMeshBlock();

        auto& mesh = meshes[packet.handle];
        mesh.mesh = std::make_unique<DefaultMesh>();
        mesh.dirty = false;
    } break;

    case UpdatePacket::Operation::Destroy: {
        assert(meshes.contains(packet.handle));
        auto& mesh = meshes[packet.handle];
        if (mesh.blockKey != kInvalidDrawBlockKey) {
            mVisualSystem->getRenderManager()->removeDrawBlock(mesh.blockKey);
        }
        meshes.erase(packet.handle);
    } break;

    case UpdatePacket::Operation::Update: {
        assert(meshes.contains(packet.handle));
        auto& mesh = meshes[packet.handle];

        bool dirty = false;
        mesh.mesh->update(std::get<DefaultMesh::UpdatePacket>(packet.data),
                          mVisualSystem->getResourceManager(), dirty);

        if (dirty) {
            mesh.dirty = true;
            if (mesh.blockKey != kInvalidDrawBlockKey) {
                mVisualSystem->getRenderManager()->removeDrawBlock(
                    mesh.blockKey);
            }
            mesh.blockKey = kInvalidDrawBlockKey;
        }
        /*
        meshBlock.vsMesh->setVertexData(meshBlock.mesh->getMesh().get(), 0, 1);

        MeshTransform cbufferData;
        cbufferData.localToWorld = meshBlock.mesh->getLocalMatrix();
        cbufferData.normalTransform =
            cbufferData.localToWorld.inverse().transpose();
        meshBlock.vsMesh->setConstantBufferData(2, &cbufferData,
                                                sizeof(MeshTransform));

        if (meshBlock.mesh->getMaterial().colormap != nullptr) {
            meshBlock.psMesh->setTexture(
                0, meshBlock.mesh->getMaterial().colormap.get());
        } else {
            Texture* fallback = mVisualSystem->getResourceManager()
                                    ->getTexture(SystemTexture_FallbackColormap)
                                    .get();
            meshBlock.psMesh->setTexture(0, fallback);
        }
        */
    } break;
    }
}

Camera* SceneManagerImpl::getMainCamera() { return activeCamera; }

} // namespace Graphics
} // namespace Engine