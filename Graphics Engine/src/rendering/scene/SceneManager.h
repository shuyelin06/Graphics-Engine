#pragma once

#include <memory>
#include <vector>

#include "datamodel/core/DMTracking.h"

namespace Engine {
namespace Graphics {
class VisualSystem;
class SceneManagerImpl;

using namespace Datamodel;

// Class SceneManager:
// Interfaces with the datamodel. Processes datamodel
// events and interfaces with the appropriate system for each event received.
class SceneManager : public Datamodel::DMListener {
  public:
    static std::unique_ptr<SceneManager> create(VisualSystem* visual_system);
    ~SceneManager();

    // Process all incoming datamodel events
    void update();

    // Datamodel::DMListener Implementation
    void onDatamodelEvent(const Datamodel::DMEvent& event) override;

  private:
    std::unique_ptr<SceneManagerImpl> mImpl;

    SceneManager();
};

} // namespace Graphics
} // namespace Engine