#pragma once

#include "math/Transform.h"

namespace Engine {
using namespace Math;

namespace Input {
// MovementHandler Class:
// Represents an object which polls the input system and
// modifies an object's transform
class MovementHandler {
  private:
    // Center of the screen
    int center_x, center_y;

    Quaternion xRotation;
    Quaternion yRotation;

    float sensitivity;

    // Target transform
    Transform* transform;

  public:
    MovementHandler(Transform* transform);
    ~MovementHandler();

    // Polls the input system to update the target transform
    void update();
};
} // namespace Input
} // namespace Engine