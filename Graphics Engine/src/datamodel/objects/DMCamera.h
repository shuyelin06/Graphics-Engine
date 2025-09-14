#pragma once

#include "../CreationCallback.h"
#include "../Object.h"

namespace Engine {
namespace Datamodel {
// DMCamera Class:
// Represents a camera in the scene.
class DMCamera : public Object, public CreationCallback<DMCamera> {
  private:
    float fov;
    float z_near, z_far;

  public:
    DMCamera();
    ~DMCamera();

    void propertyDisplay() override;

    float getFOV() const;
    float getZNear() const;
    float getZFar() const;
};

} // namespace Datamodel
} // namespace Engine