#include "LightObject.h"

namespace Engine
{
namespace Graphics
{
ShadowLightObject::ShadowLightObject(Object* object, ShadowLight* _light) : VisualObject(object) {
    light = _light;
}

ShadowLightObject::~ShadowLightObject() = default;

}
}