#include "PhysicsSystem.h"

#include "collisions/GJK.h"
#include "rendering/VisualDebug.h"

namespace Engine {
using namespace Utility;

namespace Physics {
// Constructor:
// Initializes relevant fields
PhysicsSystem::PhysicsSystem() : broadphase_tree(0.2f), stopwatch() {}

// Initialize:
// Begin the stopwatch to track delta_time.
void PhysicsSystem::initialize() { stopwatch.Reset(); }

// AddCollisionHull:
// Adds a collision hull to the physics engine with a name
void PhysicsSystem::addCollisionHull(const std::string& name,
                                     const std::vector<Vector3>& points) {
    CollisionHull* new_hull = new CollisionHull(points);

    if (collision_hulls.contains(name))
        delete collision_hulls[name];

    collision_hulls[name] = new_hull;
}

// BindPhysicsObject:
// Binds a physics object to an object.
PhysicsObject* PhysicsSystem::bindPhysicsObject(Object* object) {
    PhysicsObject* phys_obj = new PhysicsObject(object);
    object->setPhysicsObject(phys_obj);
    objects.push_back(phys_obj);
    return phys_obj;
}

// BindCollisionObject:
// Bind a collision object to a physics object
CollisionObject*
PhysicsSystem::bindCollisionObject(PhysicsObject* phys_obj,
                                   const std::string& hull_id) {
    // Create my collider
    const Transform* obj_transform = &phys_obj->object->getTransform();
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

// Update:
// Updates the physics for a scene.
void PhysicsSystem::update() {
    physicsPrepare();

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
        Transform& transform = object->object->getTransform();
        transform.offsetPosition(object->velocity * delta_time);
        object->velocity /= 2;
    }
}

// PhysicsPrepare:
// Prepares the physics system to run. Removes objects that are
// marked for destruction, and finds the amount of time that has elapsed
// since the last call.
void PhysicsSystem::physicsPrepare() {
    // Remove all PhysicsObjects marked for destruction, and free their memory.
    int head = 0;

    for (int i = 0; i < objects.size(); i++) {
        if (!objects[i]->destroy) {
            objects[head] = objects[i];
            head++;
        } else {
            delete objects[i];
            objects[i] = nullptr;
        }
    }

    objects.resize(head);

    // Determine the amount of time that has elapsed since the last
    // update() call.
    delta_time = stopwatch.Duration();
    stopwatch.Reset();
}

} // namespace Physics
} // namespace Engine