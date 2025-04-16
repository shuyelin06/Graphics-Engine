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

    // Terrain
    Terrain* terrain;

  public:
    Scene();
    ~Scene();

    // --- Object Handling ---
    const std::vector<Object*>& getObjects();
    Object& createObject();

    // Update object transforms and submit render requests
    void updateObjects();

    // --- Terrain Handling ---
    const Terrain* getTerrain() const;

    // Update the terrain based on some center position
    void updateTerrainChunks(float x, float y, float z);

  private:
    void updateObjectsHelper(Object* object, const Matrix4& m_parent);
};

} // namespace Datamodel
} // namespace Engine