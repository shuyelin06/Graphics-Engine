#include "SceneManager.h"

#include "../VisualSystem.h"

#include "rendering/terrain/VisualTerrain.h"

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

        switch (event.object_type) {
        case DMObjectTag::kTerrain:
            processTerrainEvent(event);
            break;
            // TODO: event loop + binding with other managers.
        default:
            break;
        }
    }
}

void SceneManagerImpl::onDatamodelEvent(const Datamodel::DMEvent& event) {
    std::scoped_lock<std::mutex> eventsScratchLock(mEventsScratchLock);
    mEventsScratch.emplace_back(event);
}

void SceneManagerImpl::processTerrainEvent(const DMEvent& event) {
    assert(event.object_type == DMObjectTag::kTerrain);
    VisualTerrain* visualTerrain = mVisualSystem->getVisualTerrain();
    assert(visualTerrain);

    VisualTerrain::UpdatePacket packet;

    switch (event.event_type) {
    case DMEventType::kCreated:
        packet.type = VisualTerrain::UpdatePacket::Type::kToggleTerrain;
        packet.data = true;
        break;

    case DMEventType::kDestroyed:
        packet.type = VisualTerrain::UpdatePacket::Type::kToggleTerrain;
        packet.data = false;
        break;

    case DMEventType::kPropertyUpdated:
        break;
    }

    visualTerrain->submitSceneUpdate(packet);
}

} // namespace Graphics
} // namespace Engine