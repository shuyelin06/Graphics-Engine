#pragma once

#include <unordered_map>
#include <vector>

#include "collisions/AABBTree.h"

#include "PhysicsObject.h"
#include "PhysicsTerrain.h"

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
    std::vector<PhysicsObject*> objects;
    PhysicsTerrain* terrain;

  public:
    PhysicsSystem();

    void addCollisionHull(const std::string& name,
                          const std::vector<Vector3>& points);

    // Datamodel Handling
    void onObjectCreate(Object* object);

    // Bind a PhysicsObject to an object and return it for configuration
    /*
    CollisionObject* bindCollisionObject(PhysicsObject* physics_object,
                                         const std::string& hull_id);
    PhysicsTerrain* bindTerrain(Terrain* terrain);
    */

    // (SYNC) Pull data from the datamodel
    void pullDatamodelData();
    // Updates the physics for a scene
    void update();
    // (SYNC) Push data to the datamodel
    void pushDatamodelData();

    // Raycast into the scene
    BVHRayCast raycast(const Vector3& origin, const Vector3& direction);
};
} // namespace Physics
} // namespace Engine