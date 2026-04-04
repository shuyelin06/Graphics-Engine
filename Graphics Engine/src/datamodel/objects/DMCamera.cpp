#include "DMCamera.h"

namespace Engine {
namespace Datamodel {
DMCamera::DMCamera()
    : Object("Camera"), fov(&getDMHandle(), "FOV"),
      zNear(&getDMHandle(), "ZNear"), zFar(&getDMHandle(), "ZFar") {
    fov.writeProperty(1.2f);
    zNear.writeProperty(5.f);
    zFar.writeProperty(500.f);
}
DMCamera::~DMCamera() = default;

void DMCamera::propertyDisplay() {
    // TODO
}

float DMCamera::getFOV() const { return fov.readProperty(); }
float DMCamera::getZNear() const { return zNear.readProperty(); }
float DMCamera::getZFar() const { return zFar.readProperty(); }

} // namespace Datamodel
} // namespace Engine