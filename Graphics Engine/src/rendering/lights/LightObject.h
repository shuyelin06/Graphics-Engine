#pragma once

#include "../VisualObject.h"
#include "Light.h"

namespace Engine {
namespace Graphics {
// LightObject:
// Represents a light in the engine.
class ShadowLightObject : public VisualObject {
    friend class VisualSystem;

  private:
    ShadowLight* light;

    ShadowLightObject(Object* object, ShadowLight* light);

  public:
    ~ShadowLightObject();

    // Update:
    // Updates the light's internal data with data from the datamodel.
    void pullDatamodelData() const;

  private:
    void updateLightMatrices() const;
};

} // namespace Graphics
} // namespace Engine