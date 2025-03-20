#include "Object.h"

#include <math.h>

#include "physics/PhysicsObject.h"
#include "rendering/VisualObject.h"

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

    // Objects start with no asset
    visual_object = nullptr;
    physics_object = nullptr;

    // Default transform
    transform = Transform();
}

// Destructor:
// Frees all memory allocated for this object, including children
// and components.
Object::~Object() {
    // Deallocate children
    for (Object* child : children)
        delete child;

    setVisualObject(nullptr);
    setPhysicsObject(nullptr);
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

// GetVisualObject:
// Returns the visual object currently associated with this object
const VisualObject* Object::getVisualObject() const { return visual_object; }

// SetVisualObject:
// Change the current visual object. If needed, marks the old visual object for destruction.
void Object::setVisualObject(VisualObject* visual_obj) {
    if (visual_object != nullptr)
        visual_object->destroy();
    visual_object = visual_obj;
}

// GetPhysicsObject:
// Returns the physics object currently associated with this object
const PhysicsObject* Object::getPhysicsObject() const { return physics_object; }

// SetPhysicsObject:
// Change the current physics object. If needed, marks the old physics object
// for destruction.
void Object::setPhysicsObject(PhysicsObject* phys_obj) {
    if (physics_object != nullptr)
        physics_object->destroy = true;
    physics_object = phys_obj;
}

} // namespace Datamodel
} // namespace Engine