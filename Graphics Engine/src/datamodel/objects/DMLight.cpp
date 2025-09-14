#include "DMLight.h"

namespace Engine {
namespace Datamodel {
DMLight::DMLight() : Object(), Bindable<DMLight>(this) {}
DMLight::~DMLight() = default;

} // namespace Datamodel
} // namespace Engine