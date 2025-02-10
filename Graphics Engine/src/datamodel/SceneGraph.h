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
    // Center of the scene graph. Culling / loading is performed based on this
    // center
    Vector3* center; 

    std::vector<Object*> objects;
    
public:
    Terrain* terrain; // TEMP

  public:
    SceneGraph();
    ~SceneGraph();

    // Object handling
    const std::vector<Object*>& getObjects();
    Object& createObject();

    // Update terrain (if needed) and submit render requests for each.
    // Later, the scene graph can do some culling optimizations for us. 
    void updateAndRenderTerrain();
    
    // Update object transforms and submit render requests
    // for each
    void updateAndRenderObjects();

  private:
    void updateAndRenderObjects(Object* object, const Matrix4& m_parent);
};

} // namespace Datamodel
} // namespace Engine