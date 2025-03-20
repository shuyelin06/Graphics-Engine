#include "VisualObject.h"

namespace Engine {
namespace Graphics {
VisualObject::VisualObject(Object* _object) : object(_object) {
    markedToDestroy = false;
}

VisualObject::~VisualObject() = default;

bool VisualObject::markedForDestruction() const { return markedToDestroy; }
void VisualObject::destroy() { markedToDestroy = true; }

} // namespace Graphics
} // namespace Engine