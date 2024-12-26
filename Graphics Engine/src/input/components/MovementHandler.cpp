#include "MovementHandler.h"

#include "input/InputSystem.h"
#include "input/callbacks/InputPoller.h"

namespace Engine {
namespace Input {
MovementHandler::MovementHandler(Transform* _transform) {
    transform = _transform;

    sensitivity = 1;

    xRotation = Quaternion(Vector3(), 1.f);
    yRotation = Quaternion(Vector3(), 1.f);

    center_x = 600;
    center_y = 600;
}
MovementHandler::~MovementHandler() = default;

void MovementHandler::update() {
    // Handle XYZ movement first.
    Vector3 movementVector = Vector3();

    // Poll the input system for the status of the WASDQE keys.
    // Use this to form a movement vector indicating the direction
    // to move in.
    if (InputPoller::IsSymbolActive('w'))
        movementVector += transform->forward();
    if (InputPoller::IsSymbolActive('s'))
        movementVector += transform->backward();
    if (InputPoller::IsSymbolActive('a'))
        movementVector += transform->left();
    if (InputPoller::IsSymbolActive('d'))
        movementVector += transform->right();
    if (InputPoller::IsSymbolActive('q'))
        movementVector += transform->down();
    if (InputPoller::IsSymbolActive('e'))
        movementVector += transform->up();

    // If any movement input is active, offset the object
    // position.
    // Normalize movement vector and offset position
    if (movementVector.magnitude() != 0) {
        movementVector.inplaceNormalize();
        movementVector /= 3.f;

        transform->offsetPosition(movementVector.x, movementVector.y,
                                  movementVector.z);
    }

    // Then, handle camera rotation movement.
    POINT new_pos;
    GetCursorPos(&new_pos);

    int x_delta = new_pos.x - center_x;
    int y_delta = new_pos.y - center_y;

    // Convert to Angular Displacement
    // Roll = Rotation Around X (Up/Down)
    // Pitch = Rotation Around Y (Left/Right)
    xRotation *=
        Quaternion::RotationAroundAxis(Vector3::PositiveY(), x_delta / 100.f);
    yRotation *=
        Quaternion::RotationAroundAxis(Vector3::PositiveX(), y_delta / 100.f);

    transform->setRotation(xRotation * yRotation);

    // Reset mouse to center of application
    SetCursorPos(center_x, center_y);
}
} // namespace Input
} // namespace Engine