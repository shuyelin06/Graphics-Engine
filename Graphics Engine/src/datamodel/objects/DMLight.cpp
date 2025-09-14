#include "DMLight.h"

namespace Engine {
namespace Datamodel {
DMLight::DMLight() : Object("Light"), Bindable<DMLight>(this) {}
DMLight::~DMLight() = default;

} // namespace Datamodel
} // namespace Engine