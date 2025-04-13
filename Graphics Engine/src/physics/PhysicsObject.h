#pragma once

#include "collisions/CollisionObject.h"
#include "datamodel/Object.h"

#include "collisions/GJKSupport.h"
#include "math/Vector3.h"

namespace Engine {
using namespace Datamodel;
using namespace Math;

namespace Physics {
// PhysicsObject Struct:
// Data that represents the physics state of an object in the
// datamodel.
class PhysicsObject : public Component {
    friend class PhysicsSystem;

  private:
    Vector3 acceleration;
    Vector3 velocity;

    CollisionObject* collider;

    Quaternion xRotation; // Left-Right Rotation (Z-Axis)
    Quaternion yRotation; // Up-Down Rotation (Y-Axis)
    float prev_x, prev_y;

  public:
    PhysicsObject(Object* object);
    ~PhysicsObject();

    void pollInput();
    void applyVelocity(float delta_time);
    void applyAcceleration(float delta_time);
};

} // namespace Physics
} // namespace Engine