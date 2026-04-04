#pragma once

#include <memory>
#include <vector>

#include "datamodel/core/DMTracking.h"

namespace Engine {
namespace Graphics {
class VisualSystem;
class SceneListenerImpl;

using namespace Datamodel;

// Class SceneListener:
// Interfaces with the datamodel. Processes datamodel
// events and interfaces with the appropriate system for each event received.
class SceneListener : public Datamodel::DMListener {
  public:
    static std::unique_ptr<SceneListener> create(VisualSystem* visual_system);
    ~SceneListener();

    // Process all incoming datamodel events
    void update();

    // Datamodel::DMListener Implementation
    void onDatamodelEvent(const Datamodel::DMEvent& event) override;

  private:
    std::unique_ptr<SceneListenerImpl> mImpl;

    SceneListener();
};

} // namespace Graphics
} // namespace Engine