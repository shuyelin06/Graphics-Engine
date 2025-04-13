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

    // Update:
    // Updates the light's internal data with data from the datamodel.
    void pullDatamodelData() const;

  private:
    void updateLightMatrices() const;
};

} // namespace Graphics
} // namespace Engine