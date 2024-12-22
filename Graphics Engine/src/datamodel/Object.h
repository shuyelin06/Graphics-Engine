#pragma once

#include <map>
#include <vector>

#include "math/Matrix4.h"
#include "math/Transform.h"
#include "math/Vector3.h"

namespace Engine {
using namespace Math;

namespace Datamodel {
// Object Class
// Stores data regarding a generic object in our engine.
// Every object has a parent and children. Together, their parent / child
// relationships form an entire scene.
// Unique behaviors for objects are implemented using components, which can be
// registered.
class Object {
  protected:
    // Parent & Children for the Object
    Object* parent;
    std::vector<Object*> children;

    // Transform of the object
    Transform transform;

    // (Cached) Local --> World Matrix
    Matrix4 m_local;

  public:
    // Constructor & Destructor
    Object();
    ~Object();

    // Object Hierarchy Methods
    Object* getParent() const; // Can return nullptr if parent does not exist
    std::vector<Object*>& getChildren();

    Object& createChild();

    // Transform Methods
    Transform& getTransform();

    const Matrix4& getLocalMatrix() const;
    const Matrix4& updateLocalMatrix(const Matrix4& m_parent);
};
} // namespace Datamodel
} // namespace Engine