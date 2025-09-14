#include "DMLight.h"

namespace Engine {
namespace Datamodel {
DMLight::DMLight() : Object("Light"), CreationCallback<DMLight>(this) {}
DMLight::~DMLight() = default;

} // namespace Datamodel
} // namespace Engine