#include "VisualObject.h"

namespace Engine {
namespace Graphics {
VisualObject::VisualObject(Object* _object) : object(_object) {
    destroy = false;
}

VisualObject::~VisualObject() = default;

}
} // namespace Engine