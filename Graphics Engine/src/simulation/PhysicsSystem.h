#pragma once

#include <vector>

#include "utility/Stopwatch.h"

namespace Engine {
namespace Simulation {
// PhysicsSystem Class
// Manages physics behaviors in the game engine.
class PhysicsSystem {
  private:
    // Track delta time
    Utility::Stopwatch stopwatch;

  public:
    PhysicsSystem();

    // Performs relevant initializations for the scene physics
    void initialize();

    // Updates the physics for a scene
    void update();
};
} // namespace Simulation
} // namespace Engine