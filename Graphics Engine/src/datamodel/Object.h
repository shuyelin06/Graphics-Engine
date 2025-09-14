#pragma once

#include <map>
#include <string>
#include <vector>

#include "math/Matrix4.h"
#include "math/Transform.h"
#include "math/Vector3.h"

// Used for the property display
#include "rendering/ImGui.h"

#define CLASS_ID_NONE 0

namespace Engine {
using namespace Math;

namespace Datamodel {
class DMBinding;

// Object Class
// Stores data regarding a generic object in our engine.
// Every object has a parent and children. Together, their parent / child
// relationships form an entire scene.
// In this engine, objects will implement unique behaviors through subclasses.
// All objects will have a transform.
// I've opted to move away from a component-based system, as it's needlessly
// complicated for an engine of this size. All datamodel object types should be
// prepended with "DM".
// The SceneGraph is in charge of cleaning up objects.
class Object {
  protected:
    Object* parent;
    std::vector<Object*> children;

    // Binding Outside of the Datamodel
    DMBinding* dm_binding;

    // ID = 0 for no object.
    uint16_t class_id;
    // Transform of the object
    Transform transform;
    // Cached Local --> World Matrix
    Matrix4 m_local;

    // Used in the SceneGraph Management
    bool destroy;

#if defined(_DEBUG)
    // Name (To be Displayed in the ImGui)
    std::string class_name;
#endif

  public:
    // Constructor & Destructor
    Object(const std::string& class_name);
    ~Object();

    // Accessors
#if defined(_DEBUG)
    const std::string& getClassName();
#endif

    void setClassID(uint16_t id);
    uint16_t getClassID() const;

    // Object Hierarchy Methods
    Object* getParent() const; // Can return nullptr if parent does not exist
    std::vector<Object*>& getChildren();

    void addChild(Object* object);

    void markForDestruction();
    bool shouldDestroy() const; // Used in SceneGraph

    // Datamodel Binding Methods
    void bind(DMBinding* dm_binding);
    void unbind();

    // Transform Methods
    Transform& getTransform();

    const Matrix4& getLocalMatrix() const;
    const Matrix4& updateLocalMatrix(const Matrix4& m_parent);

    // Overrideable Methods
    virtual void propertyDisplay();
};

} // namespace Datamodel
} // namespace Engine