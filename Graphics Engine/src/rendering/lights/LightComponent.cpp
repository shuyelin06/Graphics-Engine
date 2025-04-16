#include "LightComponent.h"

#include "datamodel/Object.h"

namespace Engine {
namespace Graphics {
ShadowLightComponent::ShadowLightComponent(Object* object, ShadowLight* _light)
    : Component(object) {
    light = _light;
}
ShadowLightComponent::~ShadowLightComponent() = default;

void ShadowLightComponent::update(void) {
    const Matrix4& m_world = object->getLocalMatrix();
    light->setWorldMatrix(m_world);
}

} // namespace Graphics
} // namespace Engine