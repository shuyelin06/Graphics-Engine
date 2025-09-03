#include "Object.h"

#include <math.h>
#include <unordered_map>

#include <assert.h>

#include "physics/PhysicsObject.h"

namespace Engine {
using namespace Math;

namespace Datamodel {
/* --- Constructors / Destructors --- */
// Constructor:
// Creates an object with no parent and a
// local position of (0,0,0)
Object::Object(const std::string& _class_name) {
    // Objects start with no parent and no children
    parent = nullptr;
    children = std::vector<Object*>(0);
    components = std::vector<Component*>(0);

    // Default transform
    transform = Transform();
    m_local = Matrix4::Identity();

    class_id = RegisterObjectClass(_class_name);
#if defined(_DEBUG)
    class_name = _class_name;
#endif
}

// Destructor:
// Frees all memory allocated for this object, including children
// and components.
Object::~Object() {
    // Deallocate children
    for (Object* child : children)
        delete child;

    // Mark components as invalid
    for (Component* component : components)
        component->markInvalid();
}

#if defined(_DEBUG)
const std::string& Object::getClassName() { return class_name; }
#endif

/* --- Object Class ID Methods --- */
static std::unordered_map<std::string, uint16_t> map_class_to_id =
    std::unordered_map<std::string, uint16_t>();
static uint16_t next_class_id = 1;

uint16_t Object::RegisterObjectClass(const std::string& class_name) {
    if (!map_class_to_id.contains(class_name))
        map_class_to_id[class_name] = next_class_id++;
    return map_class_to_id[class_name];
}

uint16_t Object::GetObjectClassIDByName(const std::string& class_name) {
    if (map_class_to_id.contains(class_name))
        return map_class_to_id[class_name];
    else
        return CLASS_ID_NONE;
}

/* --- Object Hierarchy Methods --- */
// GetParent:
// Returns the object's parent. Returns nullptr if the parent does
// not exist.
Object* Object::getParent() const { return parent; }

// GetChildren:
// Returns the object's children
std::vector<Object*>& Object::getChildren() { return children; }

// BindChild:
// Binds an object to this object's children vector
void Object::addChild(Object* object) {
    // TODO: Should remove pointers if object has parent.
    // Should also validate that object is not already a child.
    assert(object->parent == nullptr);
    object->parent = this;
    children.push_back(object);
}

/* --- Transform Methods --- */
// GetTransform:
// Returns the object's transform property
Transform& Object::getTransform() { return transform; }

// GetLocalToWorldMatrix:
// Returns the Object's Local -> World matrix. This can be used
// to transform points in the object's local space into world space.
const Matrix4& Object::getLocalMatrix() const { return m_local; }

// UpdateLocalToWorldMatrix:
// Update the Local -> World matrix for the object, given the
// parent's Local -> World matrix.
// This method is called in an update pre-pass every frame, and lets us
// cache the matrix to save computation.
const Matrix4& Object::updateLocalMatrix(const Math::Matrix4& m_parent) {
    // Generate local transform
    const Matrix4 m_local_transform = transform.transformMatrix();
    const Matrix4 m_parent_transform = m_parent;

    // Update local matrix.
    m_local = m_parent_transform * m_local_transform;

    return m_local;
}

// Overrideable Methods
void Object::propertyDisplay() {}

// --- Components ---
// BindComponent
// Bind a new component to the object.
int Object::bindComponent(Component* component) {
    int index = components.size();
    components.push_back(component);
    return index;
}

// RemoveComponent:
// Remove a component from the object by direct pointer to it
void Object::removeComponent(Component* component) {
    int index = -1;

    for (int i = 0; i < components.size(); i++) {
        Component* comp = components[i];
        if (comp == component) {
            index = i;
            break;
        }
    }

    if (index != -1) {
        components[index]->markInvalid();
        components.erase(components.begin() + index);
    }
}

// RemoveComponentByTag:
// Removes the first occurrence of component with the
// given tag.
void Object::removeAllComponentsWithTag(unsigned int tag) {
    components.erase(std::remove_if(components.begin(), components.end(),
                                    [tag](Component* comp) {
                                        if (comp->getTag() == tag) {
                                            comp->markInvalid();
                                            return true;
                                        } else
                                            return false;
                                    }),
                     components.end());
}

// Retrieve an object component by tag.
Component* Object::getComponent(unsigned int tag) {
    int index = -1;

    for (int i = 0; i < components.size(); i++) {
        Component* comp = components[i];
        if (comp->getTag() == tag) {
            index = i;
            break;
        }
    }

    if (index != -1)
        return components[index];
    else
        return nullptr;
}
std::vector<Component*> Object::getComponents() { return components; }

} // namespace Datamodel
} // namespace Engine