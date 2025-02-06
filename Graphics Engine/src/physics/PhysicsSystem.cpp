#include "PhysicsSystem.h"

#include "math/geometry/GJK.h"

#include "rendering/VisualDebug.h"

namespace Engine {
using namespace Utility;

namespace Physics {
// Constructor:
// Initializes relevant fields
PhysicsSystem::PhysicsSystem() : stopwatch() {}

// Initialize:
// Begin the stopwatch to track delta_time.
void PhysicsSystem::initialize() { stopwatch.Reset(); }

// BindPhysicsObject:
// Binds a physics object to an object.
PhysicsObject* PhysicsSystem::bindPhysicsObject(Object* object) {
    PhysicsObject* phys_obj = new PhysicsObject(object);
    object->setPhysicsObject(phys_obj);
    objects.push_back(phys_obj);
    return phys_obj;
}

// Update:
// Updates the physics for a scene.
void PhysicsSystem::update() {
    physicsPrepare();

    // TODO: Resolve collisions (INEFFICIENT RN)
    for (int i = 0; i < objects.size(); i++) {
        for (int j = i + 1; j < objects.size(); j++) {
            PhysicsObject* o1 = objects[i];
            PhysicsObject* o2 = objects[j];

            if (o1->collision_func != nullptr &&
                o2->collision_func != nullptr) {
                GJKSolver gjk_solver =
                    GJKSolver(o1->collision_func, o2->collision_func);

                if (gjk_solver.checkIntersection()) {
                    const Vector3 penetration = gjk_solver.penetrationVector();
                    o1->velocity += -penetration / 2;
                    o2->velocity += penetration / 2;
                }
            }
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
    // Remove all PhysicsObjects marked for destruction
    int head = 0;

    for (int i = 0; i < objects.size(); i++) {
        if (!objects[i]->destroy) {
            objects[head] = objects[i];
            head++;
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