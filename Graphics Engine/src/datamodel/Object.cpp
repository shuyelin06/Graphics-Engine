#include "Object.h"

#include <math.h>
#include <unordered_map>

#include <assert.h>

#include "DMBinding.h"
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
    dm_binding = nullptr;

    // Default transform
    transform = Transform();
    m_local = Matrix4::Identity();

    destroy = false;

    class_id = RegisterObjectClass(_class_name);

#if defined(_DEBUG)
    class_name = _class_name;
#endif
}

// Destructor:
// Frees all memory allocated for this object, including children
// and components.
Object::~Object() {
    // Unbind
    if (dm_binding != nullptr)
        dm_binding->unbind();

    // Deallocate children
    for (Object* child : children)
        delete child;
}

#if defined(_DEBUG)
const std::string& Object::getClassName() { return class_name; }
#endif

uint16_t Object::getClassID() const { return class_id; }

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

uint16_t Object::GetTotalClassCount() { return next_class_id; }

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

void Object::markForDestruction() { destroy = true; }
bool Object::shouldDestroy() const { return destroy; }

/* --- Datamodel Bindings --- */
void Object::bind(DMBinding* _dm_binding) { dm_binding = _dm_binding; }
void Object::unbind() {
    if (dm_binding != nullptr) {
        dm_binding = nullptr;
        destroy = true;
    }
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

} // namespace Datamodel
} // namespace Engine