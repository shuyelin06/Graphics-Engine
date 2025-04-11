#include "MovementHandler.h"

#include "input/InputState.h"
#include "input/InputSystem.h"

namespace Engine {
namespace Input {
MovementHandler::MovementHandler(Transform* _transform) {
    transform = _transform;

    sensitivity = 5.0f;

    xRotation = Quaternion();
    yRotation = Quaternion();

    prev_x = center_x = 0;
    prev_y = center_y = 0;
}
MovementHandler::~MovementHandler() = default;

void MovementHandler::update() {
    // Handle XYZ movement first.
    Vector3 movementVector = Vector3();

    // Poll the input system for the status of the WASDQE keys.
    // Use this to form a movement vector indicating the direction
    // to move in.
    if (InputState::IsSymbolActive(KEY_W))
        movementVector += transform->forward();
    if (InputState::IsSymbolActive(KEY_S))
        movementVector += transform->backward();
    if (InputState::IsSymbolActive(KEY_A))
        movementVector += transform->left();
    if (InputState::IsSymbolActive(KEY_D))
        movementVector += transform->right();
    if (InputState::IsSymbolActive(KEY_Q))
        movementVector += transform->down();
    if (InputState::IsSymbolActive(KEY_E))
        movementVector += transform->up();

    // If any movement input is active, offset the object
    // position.
    // Normalize movement vector and offset position
    if (movementVector.magnitude() != 0) {
        constexpr float MOVEMENT_SPEED = 3.f;

        movementVector.inplaceNormalize();
        movementVector *= MOVEMENT_SPEED;

        transform->offsetPosition(movementVector.x, movementVector.y,
                                  movementVector.z);
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
                                                    x_delta * sensitivity);
        yRotation *= Quaternion::RotationAroundAxis(Vector3::PositiveX(),
                                                    y_delta * sensitivity);

        transform->setRotation(xRotation * yRotation);
    }

    prev_x = new_pos_x;
    prev_y = new_pos_y;

    // Reset mouse to center of application
    // SetCursorPos(center_x, center_y);
}
} // namespace Input
} // namespace Engine