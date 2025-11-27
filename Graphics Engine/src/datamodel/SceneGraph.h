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

#if defined(IMGUI_ENABLED)
    Object* selected_object;
    bool show_property_window;

    int imGuiTraverseHierarchy(Object* object, int next_id);
#endif(IMGUI_ENABLED)

  public:
    Scene();
    ~Scene();

    // Displays the object hierarchy in the "Scene" menu
    // of the ImGui display.
    // Does nothing if IMGUI_ENABLED is not defined (see ImGui.h)
    void imGuiDisplay();

    // --- Object Handling ---
    void addObject(Object* object);
    const std::vector<Object*>& getObjects() const;

    // Update objects and clean up invalid ones
    void updateAndCleanObjects();
};

} // namespace Datamodel
} // namespace Engine