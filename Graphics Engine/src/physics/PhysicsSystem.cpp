#include "PhysicsSystem.h"

#include "collisions/GJK.h"
#include "rendering/VisualDebug.h"

namespace Engine {
using namespace Utility;

namespace Physics {
// Constructor:
// Initializes relevant fields
PhysicsSystem::PhysicsSystem() : broadphase_tree(0.2f), stopwatch() {
    stopwatch.Reset();

    DMPhysics::ConnectToCreation([this](Object* obj) { onObjectCreate(obj); });

    terrain = nullptr;
}

// AddCollisionHull:
// Adds a collision hull to the physics engine with a name
void PhysicsSystem::addCollisionHull(const std::string& name,
                                     const std::vector<Vector3>& points) {
    CollisionHull* new_hull = new CollisionHull(points);

    if (collision_hulls.contains(name))
        delete collision_hulls[name];

    collision_hulls[name] = new_hull;
}

// Datamodel Handling
void PhysicsSystem::onObjectCreate(Object* object) {
    if (object->getClassID() == DMPhysics::ClassID()) {

        PhysicsObject* phys_obj = new PhysicsObject(object);
        objects.push_back(phys_obj);
    }
}

/*
// BindCollisionObject:
// Bind a collision object to a physics object
CollisionObject*
PhysicsSystem::bindCollisionObject(PhysicsObject* phys_obj,
                                   const std::string& hull_id) {
    // Create my collider
    const Transform* obj_transform = &phys_obj->getObject()->getTransform();
    CollisionObject* collider =
        new CollisionObject(phys_obj, obj_transform, collision_hulls[hull_id]);

    // Free previous collider, if it exists
    if (phys_obj->collider != nullptr) {
        broadphase_tree.remove(&phys_obj->collider->broadphase_aabb);
        delete phys_obj->collider;
        phys_obj->collider = nullptr;
    }

    // Add collider and register it into the broadphase tree
    phys_obj->collider = collider;
    broadphase_tree.add(&phys_obj->collider->broadphase_aabb);

    return collider;
}

PhysicsTerrain* PhysicsSystem::bindTerrain(Terrain* _terrain) {
    if (terrain != nullptr)
        delete terrain;
    terrain = new PhysicsTerrain(_terrain);
    return terrain;
}
*/

// PullDatamodelData:
// Pulls a copy of data from the datamodel, for the physics
// system to operate on.
void PhysicsSystem::pullDatamodelData() {
    // Remove all PhysicsObjects marked for destruction, and free their memory.
    // objects.cleanAndUpdate();

    // Pull datamodel data
    for (PhysicsObject* obj : objects)
        obj->pullDatamodelData();

    if (terrain != nullptr)
        terrain->pullTerrainBVHs();

    // Determine the amount of time that has elapsed since the last
    // update() call.
    delta_time = stopwatch.Duration();
    stopwatch.Reset();
}

// Update:
// Updates the physics for a scene.
void PhysicsSystem::update() {
    // Poll Input
    for (PhysicsObject* obj : objects)
        obj->pollInput();

    // Update all AABBs
    for (PhysicsObject* obj : objects) {
        if (obj->collider != nullptr) {
            obj->collider->updateBroadphaseAABB();
#if defined(_DEBUG)
            obj->collider->debugDrawCollider();
#endif
        }
    }

    // Collision Broadphase:
    // Use the dynamic AABB tree to find colliders whose AABB is colliding.
    broadphase_tree.update();
    const std::vector<ColliderPair>& collision_pairs =
        broadphase_tree.computeColliderPairs();

    // DEBUG:
#if defined(_DEBUG)
    broadphase_tree.debugDrawTree();
#endif

    // Collision Test:
    // For each pair, check that their colliders are actually intersecting.
    // If they are, then resolve the collision.
    for (const ColliderPair& pair : collision_pairs) {
        CollisionObject* c1 = pair.aabb_1->collider;
        CollisionObject* c2 = pair.aabb_1->collider;

        GJKSolver gjk_solver = GJKSolver(c1, c2);

        if (gjk_solver.checkIntersection()) {
            const Vector3 penetration = gjk_solver.penetrationVector();
            c1->phys_object->velocity += -penetration;
            c2->phys_object->velocity += penetration;
        }
    }

    // Iterate through and clear
    // Apply acceleration and velocity to all objects
    for (PhysicsObject* object : objects) {
        object->applyAcceleration(delta_time);
        object->applyVelocity(delta_time);
    }
}

// PushDatamodelData:
// Pushes data to the datamodel.
void PhysicsSystem::pushDatamodelData() {
    for (PhysicsObject* obj : objects)
        obj->push();
}

BVHRayCast PhysicsSystem::raycast(const Vector3& origin,
                                  const Vector3& direction) {
    return terrain->getTerrainTLS().raycast(origin, direction);
}

} // namespace Physics
} // namespace Engine