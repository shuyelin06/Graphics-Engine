#pragma once

#include "../core/DMCore.h"

#include "../Bindable.h"
#include "../Object.h"

namespace Engine {
namespace Datamodel {
// DMCamera Class:
// Represents a camera in the scene.
class DMCamera : public Object {
  private:
    DMTrackedProperty<float> fov;
    DMTrackedProperty<float> zNear;
    DMTrackedProperty<float> zFar;

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