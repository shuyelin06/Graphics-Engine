#include "SceneManager.h"

#include "../VisualSystem.h"

#include "rendering/terrain/TerrainManager.h"

namespace Engine {
namespace Graphics {
using DMHandle = Datamodel::DMObjectHandle;
using DMEvent = Datamodel::DMEvent;

class SceneManagerImpl {
  private:
    VisualSystem* mVisualSystem;

    std::vector<DMEvent> mEventsScratch;
    std::mutex mEventsScratchLock;
    std::vector<DMEvent> mEvents;

  public:
    SceneManagerImpl(VisualSystem* visualSystem);
    ~SceneManagerImpl();

    void update();

    // Datamodel::DMListener Implementation
    void onDatamodelEvent(const Datamodel::DMEvent& event);

  private:
    void processTerrainEvent(const DMEvent& event);
};

SceneManager::SceneManager() = default;
SceneManager::~SceneManager() = default;

std::unique_ptr<SceneManager> SceneManager::create(VisualSystem* visualSystem) {
    std::unique_ptr<SceneManager> sceneManager =
        std::unique_ptr<SceneManager>(new SceneManager());
    sceneManager->mImpl = std::make_unique<SceneManagerImpl>(visualSystem);
    return std::move(sceneManager);
}

void SceneManager::update() { mImpl->update(); }

void SceneManager::onDatamodelEvent(const Datamodel::DMEvent& event) {
    mImpl->onDatamodelEvent(event);
}

SceneManagerImpl::SceneManagerImpl(VisualSystem* _visualSystem) {
    mVisualSystem = _visualSystem;
}
SceneManagerImpl::~SceneManagerImpl() = default;

void SceneManagerImpl::update() {
    {
        std::scoped_lock<std::mutex> eventsScratchLock(mEventsScratchLock);
        std::swap(mEvents, mEventsScratch);
        mEventsScratch.clear();
    }

    while (!mEvents.empty()) {
        const DMEvent event = mEvents.back();
        mEvents.pop_back();

        if (event.object_type == "Terrain") {
            processTerrainEvent(event);
        }
    }
}

void SceneManagerImpl::onDatamodelEvent(const Datamodel::DMEvent& event) {
    std::scoped_lock<std::mutex> eventsScratchLock(mEventsScratchLock);
    mEventsScratch.emplace_back(event);
}

void SceneManagerImpl::processTerrainEvent(const DMEvent& event) {
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

} // namespace Graphics
} // namespace Engine