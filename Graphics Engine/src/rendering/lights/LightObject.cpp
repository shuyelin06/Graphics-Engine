#include "LightObject.h"

namespace Engine {
namespace Graphics {
ShadowLightObject::ShadowLightObject(Object* object, ShadowLight* _light)
    : VisualObject(object) {
    light = _light;
}
ShadowLightObject::~ShadowLightObject() = default;

void ShadowLightObject::pullDatamodelData(void) const { updateLightMatrices(); }

void ShadowLightObject::updateLightMatrices(void) const {
    const Matrix4& m_world = object->getLocalMatrix();
    light->setWorldMatrix(m_world);
}

} // namespace Graphics
} // namespace Engine