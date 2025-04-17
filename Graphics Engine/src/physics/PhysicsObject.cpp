#include "PhysicsObject.h"

#include "input/InputState.h"

namespace Engine {
using namespace Input;
namespace Physics {
PhysicsObject::PhysicsObject(Object* _object) : Component(_object) {
    acceleration = Vector3(0, 0, 0);
    velocity = Vector3(0, 0, 0);

    collider = nullptr;
}
PhysicsObject::~PhysicsObject() = default;

void PhysicsObject::pull() { transform = object->getTransform(); }
void PhysicsObject::push() { object->getTransform() = transform; }

void PhysicsObject::pollInput() {
    // Poll the input system for the status of the WASDQE keys.
    // Use this to form a movement vector indicating the direction
    // to move in.
    Vector3 movementVector = Vector3(0, 0, 0);

    if (InputState::IsSymbolActive(KEY_W))
        movementVector += transform.forward();
    if (InputState::IsSymbolActive(KEY_S))
        movementVector += transform.backward();
    if (InputState::IsSymbolActive(KEY_A))
        movementVector += transform.left();
    if (InputState::IsSymbolActive(KEY_D))
        movementVector += transform.right();
    if (InputState::IsSymbolActive(KEY_Q))
        movementVector += transform.down();
    if (InputState::IsSymbolActive(KEY_E))
        movementVector += transform.up();

    // This movement vector determines our acceleration.
    constexpr float ACCELERATION = 20.0f;
    constexpr float DECAY = 10.f;

    if (movementVector.magnitude() != 0) {
        acceleration = movementVector.unit() * ACCELERATION;
    } else {
        if (velocity.magnitude() > 0)
            acceleration = -velocity.unit() * DECAY;
        else
            acceleration = Vector3(0, 0, 0);
    }

    // Then, handle camera rotation movement.
    const float new_pos_x = InputState::DeviceXCoordinate();
    const float new_pos_y = InputState::DeviceYCoordinate();

    if (InputState::IsSymbolActive(DEVICE_ALT_INTERACT)) {
        const float x_delta = new_pos_x - prev_x;
        const float y_delta = prev_y - new_pos_y;

        // Convert to Angular Displacement
        // Roll = Rotation Around X (Up/Down)
        // Pitch = Rotation Around Y (Left/Right)
        xRotation *= Quaternion::RotationAroundAxis(Vector3::PositiveY(),
                                                    x_delta * 5.0f);
        yRotation *= Quaternion::RotationAroundAxis(Vector3::PositiveX(),
                                                    y_delta * 5.0f);

        transform.setRotation(xRotation * yRotation);
    }

    prev_x = new_pos_x;
    prev_y = new_pos_y;
}

void PhysicsObject::applyVelocity(float delta_time) {
    transform.offsetPosition(velocity * delta_time);
}
void PhysicsObject::applyAcceleration(float delta_time) {
    velocity += acceleration * delta_time;

    // Cap velocity
    constexpr float TERMINAL_VELOCITY = 40.0f;
    const float length = velocity.magnitude();
    if (length > TERMINAL_VELOCITY) {
        velocity *= TERMINAL_VELOCITY / length;
    }
}

} // namespace Physics
} // namespace Engine