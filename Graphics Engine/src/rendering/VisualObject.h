#pragma once

#include "datamodel/Object.h"

namespace Engine {
using namespace Datamodel;

namespace Graphics {
// VisualObject Class:
// Represents the renderable state of an object in the datamodel.
// This can include:
//  > The object's status as a light
//  > Mesh information for the object
class VisualObject {
    friend class VisualSystem;
    friend void Object::setVisualObject(VisualObject* visual_obj);

  protected:
    Object* const object;

    // Marks if the PhysicsObject should be destroyed or not
    // by the PhysicsSystem
    bool destroy;

    VisualObject(Object* object);

  public:
    ~VisualObject();
};

} // namespace Graphics
} // namespace Engine