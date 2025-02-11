#pragma once

#include <vector>

#include "collisions/AABBTree.h"

#include "PhysicsObject.h"
#include "utility/Stopwatch.h"

namespace Engine {
namespace Physics {
// PhysicsSystem Class
// Manages physics behaviors in the game engine.
class PhysicsSystem {
  private:
    // Track delta time
    Utility::Stopwatch stopwatch;
    float delta_time;

    // All physics object the engine is in control of
    std::vector<PhysicsObject*> objects;
    
    // Dynamic AABB tree for the collision broad-phase
    // AABBTree collision_tree;

  public:
    PhysicsSystem();

    // Performs relevant initializations for the scene physics
    void initialize();

    // Bind a PhysicsObject to an object and return it for configuration
    PhysicsObject* bindPhysicsObject(Object* object);

    // Updates the physics for a scene
    void update();

  private:
    void physicsPrepare(); // Prepare for Physics
};
} // namespace Physics
} // namespace Engine