#include "SceneManager.h"

#include "../VisualSystem.h"

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
};

SceneManager::SceneManager() = default;
SceneManager::~SceneManager() = default;

std::unique_ptr<SceneManager> SceneManager::create(VisualSystem* visualSystem) {
    std::unique_ptr<SceneManager> sceneManager =
        std::make_unique<SceneManager>();
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

        switch (event.event_type) {
            // TODO: event loop + binding with other managers.
        }
    }
}

void SceneManagerImpl::onDatamodelEvent(const Datamodel::DMEvent& event) {
    std::scoped_lock<std::mutex> eventsScratchLock(mEventsScratchLock);
    mEventsScratch.emplace_back(event);
}

} // namespace Graphics
} // namespace Engine