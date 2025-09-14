#pragma once

#include "datamodel/objects/DMPhysics.h"
#include "datamodel/DMBinding.h"

#include "collisions/CollisionObject.h"
#include "collisions/CollisionObject.h"

#include "collisions/GJKSupport.h"
#include "math/Vector3.h"

namespace Engine {
using namespace Datamodel;
using namespace Math;

namespace Physics {
// PhysicsObject Struct:
// Data that represents the physics state of an object in the
// datamodel.
class PhysicsObject : public DMBinding {
    friend class PhysicsSystem;

  protected:
    Object* object; // HACKY

    Transform transform;

    Vector3 acceleration;
    Vector3 velocity;

    CollisionObject* collider;

    Quaternion xRotation; // Left-Right Rotation (Z-Axis)
    Quaternion yRotation; // Up-Down Rotation (Y-Axis)
    float prev_x, prev_y;

    void pullDatamodelDataImpl(Object* obj) override;

  public:
    PhysicsObject(Object* object);
    ~PhysicsObject();

    // Pull and push data from this component
    // and the datamodel.
    void push();

    void pollInput();
    void applyVelocity(float delta_time);
    void applyAcceleration(float delta_time);
};

} // namespace Physics
} // namespace Engine