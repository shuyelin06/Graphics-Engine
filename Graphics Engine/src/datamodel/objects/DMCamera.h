#pragma once

#include "../Object.h"

namespace Engine {
namespace Datamodel {
// DMCamera Class:
// Represents a camera in the scene.
class DMCamera : public Object {
  private:
    float fov;
    float z_near, z_far;

  public:
    DMCamera();
    ~DMCamera();

    void propertyDisplay() override;
};

} // namespace Datamodel
} // namespace Engine