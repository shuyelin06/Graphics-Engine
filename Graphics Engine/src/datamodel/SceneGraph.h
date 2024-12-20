#pragma once

#include <vector>

#include "Object.h"
#include "Terrain.h"

namespace Engine {
namespace Datamodel {

// Class SceneGraph:
// Stores and manages all objects in the scene. Objects are stored in a
// tree-like hierarchy, Parent <--> Children Where all children node transforms
// are based off the local coordinate system of their parent. The reference
// coordinate system of all nodes without a parent is the world coordinate
// system.
class SceneGraph {
  private:
    std::vector<Object*> objects;
    Terrain* terrain;

  public:
    SceneGraph();
    ~SceneGraph();

    // Object handling
    const std::vector<Object*>& getObjects();

    Object& createObject();

    // Terrain handling
    const Terrain* getTerrain() const;
    Terrain* getTerrain();

    // Update and cache object transforms in the SceneGraph
    void updateObjectTransforms();

  private:
    void updateObjectTransforms(Object* object, const Matrix4& m_parent);
};

} // namespace Datamodel
} // namespace Engine