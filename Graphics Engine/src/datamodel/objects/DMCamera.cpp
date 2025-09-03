#include "DMCamera.h"

namespace Engine {
namespace Datamodel {
DMCamera::DMCamera() : Object("Camera") {
    fov = 1.2f;

    z_near = 5.f;
    z_far = 500.f;
}
DMCamera::~DMCamera() = default;

void DMCamera::propertyDisplay() { ImGui::SliderFloat("FOV", &fov, 0.1f, 2.f); }

} // namespace Datamodel
} // namespace Engine