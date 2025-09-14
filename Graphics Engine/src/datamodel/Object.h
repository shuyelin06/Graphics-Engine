#pragma once

#include <map>
#include <vector>

#include "Component.h"
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

    // DEPRECATE-- Components
    std::vector<Component*> components;

  public:
    // Constructor & Destructor
    Object(const std::string& class_name);
    ~Object();

    // Accessors
#if defined(_DEBUG)
    const std::string& getClassName();
#endif

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

    // --- Component Methods ---
    // Bind a new component to the object.
    int bindComponent(Component* component);

    // Remove a component (or components) from the object.
    // Components removed are marked invalid.
    void removeComponent(Component* component);
    void removeAllComponentsWithTag(unsigned int tag);

    // Retrieve an object component by tag.
    Component* getComponent(unsigned int tag);
    std::vector<Component*> getComponents();

    // Obtain IDs for Class Names
    // Requires that a class register itself.
    static uint16_t GetObjectClassIDByName(const std::string& class_name);
    static uint16_t GetTotalClassCount();

  private:
    static uint16_t RegisterObjectClass(const std::string& class_name);
};

} // namespace Datamodel
} // namespace Engine