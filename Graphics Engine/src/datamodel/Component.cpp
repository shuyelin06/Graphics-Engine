#include "Component.h"

#include "Object.h"

namespace Engine {
namespace Datamodel {
std::unordered_map<std::string, unsigned int> Component::tag_map =
    std::unordered_map<std::string, unsigned int>();
unsigned int Component::new_tag = 1;
unsigned int Component::registerNewTag(const std::string& name) {
    if (tag_map.contains(name))
        return COMPONENT_TAG_NONE;
    else {
        const unsigned int tag = new_tag++;
        tag_map[name] = tag;
        return tag;
    }
}
unsigned int Component::getTag(const std::string& name) {
    if (tag_map.contains(name))
        return COMPONENT_TAG_NONE;
    else
        return tag_map[name];
}

Component::Component(Object* _object) : object(_object) {
    tag = COMPONENT_TAG_NONE;
    valid = true;
}
Component::~Component() {
    // On destroying the component, remove it from its parent object.
    // We know the parent object is still alive if the component is still valid.
    if (valid)
        object->removeComponent(this);
}

bool Component::isValid() const { return valid; }
unsigned int Component::getTag() const { return tag; }

const Object* Component::getObject() const { return object; }
Object* Component::getObject() { return object; }

void Component::markInvalid() { valid = false; }
void Component::update() {}

} // namespace Datamodel
} // namespace Engine