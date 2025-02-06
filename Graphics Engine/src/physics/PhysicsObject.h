#pragma once

#include "datamodel/Object.h"

#include "math/geometry/GJKSupport.h"
#include "math/Vector3.h"

namespace Engine {
using namespace Datamodel;
using namespace Math;

namespace Physics {
// PhysicsObject Struct:
// Data that represents the physics state of an object in the
// datamodel.
class PhysicsObject {
friend class PhysicsSystem;
friend class Object;

    Object* const object;

    Vector3 velocity;
    GJKSupportFunc* collision_func;

    // Marks if the PhysicsObject should be destroyed or not
    // by the PhysicsSystem
    bool destroy; 

    PhysicsObject(Object* object);

public:
    ~PhysicsObject();

    void setVelocity(const Vector3& velocity);
    void setCollisionFunction(GJKSupportFunc* collision_func);
};

} // namespace Physics
} // namespace Engine