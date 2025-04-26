#pragma once

#include "Light.h"
#include "datamodel/Component.h"

namespace Engine {
using namespace Datamodel;
namespace Graphics {
// LightObject:
// Represents a light in the engine.
class ShadowLightComponent : public Component {
  private:
    ShadowLight* light;

  public:
    ShadowLightComponent(Object* object, ShadowLight* light);
    ~ShadowLightComponent();

    // OVERRIDE: Updates the light's internal data with data from the datamodel.
    void update();
};

} // namespace Graphics
} // namespace Engine