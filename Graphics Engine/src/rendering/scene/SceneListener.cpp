#include "SceneListener.h"

#include "../VisualSystem.h"

#include "rendering/terrain/TerrainManager.h"

namespace Engine {
namespace Graphics {
using DMHandle = Datamodel::DMObjectHandle;
using DMEvent = Datamodel::DMEvent;

class SceneListenerImpl {
  private:
    VisualSystem* mVisualSystem;

    std::vector<DMEvent> mEventsScratch;
    std::mutex mEventsScratchLock;
    std::vector<DMEvent> mEvents;

  public:
    SceneListenerImpl(VisualSystem* visualSystem);
    ~SceneListenerImpl();

    void update();

    // Datamodel::DMListener Implementation
    void onDatamodelEvent(const Datamodel::DMEvent& event);

  private:
    void processTerrainEvent(const DMEvent& event);
    void processMeshEvent(const DMEvent& event);
    void processCameraEvent(const DMEvent& event);
};

SceneListener::SceneListener() = default;
SceneListener::~SceneListener() = default;

std::unique_ptr<SceneListener>
SceneListener::create(VisualSystem* visualSystem) {
    std::unique_ptr<SceneListener> sceneManager =
        std::unique_ptr<SceneListener>(new SceneListener());
    sceneManager->mImpl = std::make_unique<SceneListenerImpl>(visualSystem);
    return std::move(sceneManager);
}

void SceneListener::update() { mImpl->update(); }

void SceneListener::onDatamodelEvent(const Datamodel::DMEvent& event) {
    mImpl->onDatamodelEvent(event);
}

SceneListenerImpl::SceneListenerImpl(VisualSystem* _visualSystem) {
    mVisualSystem = _visualSystem;
}
SceneListenerImpl::~SceneListenerImpl() = default;

void SceneListenerImpl::update() {
    {
        std::scoped_lock<std::mutex> eventsScratchLock(mEventsScratchLock);
        std::swap(mEvents, mEventsScratch);
        mEventsScratch.clear();
    }

    while (!mEvents.empty()) {
        const DMEvent event = mEvents.back();
        mEvents.pop_back();

        if (event.object_type == "Camera") {
            processCameraEvent(event);
        } else if (event.object_type == "Mesh") {
            processMeshEvent(event);
        } else if (event.object_type == "Terrain") {
            processTerrainEvent(event);
        }
    }
}

void SceneListenerImpl::onDatamodelEvent(const Datamodel::DMEvent& event) {
    std::scoped_lock<std::mutex> eventsScratchLock(mEventsScratchLock);
    mEventsScratch.emplace_back(event);
}

void SceneListenerImpl::processTerrainEvent(const DMEvent& event) {
    assert(event.object_type == "Terrain");
    TerrainManager* visualTerrain = mVisualSystem->getVisualTerrain();
    assert(visualTerrain);

    TerrainManager::UpdatePacket packet;

    switch (event.event_type) {
    case DMEventType::kCreated:
        packet.type = TerrainManager::UpdatePacket::Type::kToggleTerrain;
        packet.data = true;
        break;

    case DMEventType::kDestroyed:
        packet.type = TerrainManager::UpdatePacket::Type::kToggleTerrain;
        packet.data = false;
        break;

    case DMEventType::kPropertyUpdated:
        if (event.property_tag == "Seed") {
            packet.type = TerrainManager::UpdatePacket::Type::kPropertySeed;
            packet.data = std::get<uint32_t>(event.property_data);
        }
        break;
    }

    visualTerrain->submitSceneUpdate(packet);
}

void SceneListenerImpl::processCameraEvent(const DMEvent& event) {
    assert(event.object_type == "Camera");
    SceneManager* sceneManager = mVisualSystem->getSceneManager();

    SceneManager::UpdatePacket packet;
    Camera::UpdatePacket& data = packet.data.emplace<Camera::UpdatePacket>();

    packet.handle = event.object;

    switch (event.event_type) {
    case DMEventType::kCreated:
        packet.operation = SceneManager::UpdatePacket::Create;
        break;

    case DMEventType::kDestroyed:
        packet.operation = SceneManager::UpdatePacket::Destroy;
        break;

    case DMEventType::kPropertyUpdated: {
        packet.operation = SceneManager::UpdatePacket::Update;
        if (event.property_tag == "FOV") {
            data.type = Camera::UpdatePacket::Property::FOV;
            data.data = std::get<float>(event.property_data);
        } else if (event.property_tag == "ZNear") {
            data.type = Camera::UpdatePacket::Property::ZNear;
            data.data = std::get<float>(event.property_data);
        } else if (event.property_tag == "ZFar") {
            data.type = Camera::UpdatePacket::Property::ZFar;
            data.data = std::get<float>(event.property_data);
        } else if (event.property_tag == "LocalMatrix") {
            data.type = Camera::UpdatePacket::Property::LocalMatrix;
            data.data = std::get<Matrix4>(event.property_data);
        }
    } break;
    }

    sceneManager->submitUpdatePacket(packet);
}

void SceneListenerImpl::processMeshEvent(const DMEvent& event) {
    assert(event.object_type == "Mesh");
    SceneManager* sceneManager = mVisualSystem->getSceneManager();

    SceneManager::UpdatePacket packet;
    RenderableMesh::UpdatePacket& data =
        packet.data.emplace<RenderableMesh::UpdatePacket>();

    packet.handle = event.object;

    switch (event.event_type) {
    case DMEventType::kCreated:
        packet.operation = SceneManager::UpdatePacket::Create;
        break;

    case DMEventType::kDestroyed:
        packet.operation = SceneManager::UpdatePacket::Destroy;
        break;

    case DMEventType::kPropertyUpdated: {
        using Property = RenderableMesh::UpdatePacket::Property;
        packet.operation = SceneManager::UpdatePacket::Update;
        if (event.property_tag == "MeshName") {
            data.type = Property::MeshName;
            data.data = std::get<std::string>(event.property_data);
        } else if (event.property_tag == "ColormapName") {
            data.type = Property::ColorMapName;
            data.data = std::get<std::string>(event.property_data);
        }
    } break;
    }

    sceneManager->submitUpdatePacket(packet);
}

} // namespace Graphics
} // namespace Engine