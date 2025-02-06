#pragma once

#include <vector>

#include "Object.h"
#include "Terrain.h"
#include "datamodel/TerrainConfig.h"

#include "rendering/RenderRequest.h"

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
    Terrain* terrain_chunks[CHUNK_X_LIMIT][CHUNK_Z_LIMIT];

  public:
    SceneGraph();
    ~SceneGraph();

    // Object handling
    const std::vector<Object*>& getObjects();

    Object& createObject();

    // Terrain handling
    const Terrain* getTerrain(int x, int z) const;
    Terrain* getTerrain(int x, int z);

    // Update terrain (if needed) and submit render requests for each.
    // Later, the scene graph can do some culling optimizations for us. 
    void updateAndRenderTerrain(std::vector<TerrainRenderRequest>& requests);
    
    // Update object transforms and submit render requests
    // for each
    void updateAndRenderObjects();

  private:
    void updateAndRenderObjects(Object* object, const Matrix4& m_parent);
};

} // namespace Datamodel
} // namespace Engine