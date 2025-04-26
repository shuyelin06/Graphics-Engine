#include "Object.h"

#include <math.h>

#include "physics/PhysicsObject.h"

namespace Engine {
using namespace Math;

namespace Datamodel {
/* --- Constructors / Destructors --- */
// Constructor:
// Creates an object with no parent and a
// local position of (0,0,0)
Object::Object() {
    // Objects start with no parent and no children
    parent = nullptr;
    children = std::vector<Object*>(0);
    components = std::vector<Component*>(0);

    // Default transform
    transform = Transform();
    m_local = Matrix4::Identity();
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

/* --- Object Hierarchy Methods --- */
// GetParent:
// Returns the object's parent. Returns nullptr if the parent does
// not exist.
Object* Object::getParent() const { return parent; }

// GetChildren:
// Returns the object's children
std::vector<Object*>& Object::getChildren() { return children; }

// CreateChild:
// Creates a child of the object and returns it
Object& Object::createChild() {
    Object* child = new Object();
    child->parent = this;
    children.push_back(child);

    return *child;
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

} // namespace Datamodel
} // namespace Engine