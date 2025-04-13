#pragma once

#include <unordered_map>
#include <vector>

#include "collisions/AABBTree.h"

#include "PhysicsObject.h"
#include "datamodel/ComponentHandler.h"

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

    // Dynamic AABB tree for the collision broad-phase
    AABBTree broadphase_tree;

    std::unordered_map<std::string, CollisionHull*> collision_hulls;

    // All physics object the engine is in control of
    ComponentHandler<PhysicsObject> objects;

  public:
    PhysicsSystem();

    // Performs relevant initializations for the scene physics
    void initialize();

    void addCollisionHull(const std::string& name,
                          const std::vector<Vector3>& points);

    // Bind a PhysicsObject to an object and return it for configuration
    PhysicsObject* bindPhysicsObject(Object* object);

    CollisionObject* bindCollisionObject(PhysicsObject* physics_object,
                                         const std::string& hull_id);

    // Updates the physics for a scene
    void update();

  private:
    void physicsPrepare(); // Prepare for Physics
};
} // namespace Physics
} // namespace Engine