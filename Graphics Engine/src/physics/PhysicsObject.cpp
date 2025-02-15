#include "PhysicsObject.h"

namespace Engine {
namespace Physics {
PhysicsObject::PhysicsObject(Object* _object) :
    object(_object) {
    
    velocity = Vector3(0,0,0);

    collider = nullptr;

    destroy = false;
}
PhysicsObject::~PhysicsObject() = default;

void PhysicsObject::setVelocity(const Vector3& _velocity) {
    velocity = _velocity;
}

}
} // namespace Engine