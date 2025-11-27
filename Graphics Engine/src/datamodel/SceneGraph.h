#pragma once

#include <vector>

#include "Object.h"
#include "terrain/Terrain.h"

namespace Engine {
using namespace Math;

namespace Datamodel {
// Class SceneGraph:
// Stores and manages all objects in the scene. Objects are stored in a
// tree-like hierarchy, Parent <--> Children Where all children node transforms
// are based off the local coordinate system of their parent. The reference
// coordinate system of all nodes without a parent is the world coordinate
// system.
class Scene {
  private:
    std::vector<Object*> objects;

#if defined(_DEBUG)
    Object* selected_object;
#endif(_DEBUG)

  public:
    Scene();
    ~Scene();

#if defined(_DEBUG)
    // Displays the object hierarchy in the "Scene" menu
    // of the ImGui display.
    void imGuiDisplay();
#endif(_DEBUG)

    // --- Object Handling ---
    void addObject(Object* object);
    const std::vector<Object*>& getObjects() const;

    // Update objects and clean up invalid ones
    void updateAndCleanObjects();
};

} // namespace Datamodel
} // namespace Engine