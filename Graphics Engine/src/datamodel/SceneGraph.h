#pragma once

#include <vector>

#include "Object.h"
#include "Terrain.h"

#include "rendering/RenderRequest.h"

constexpr int CHUNK_X_LIMIT = 5;
constexpr int CHUNK_Z_LIMIT = 5;

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

    // Update object transforms and submit render requests
    // for each
    void updateAndRenderObjects(std::vector<AssetRenderRequest>& requests);

  private:
    void updateAndRenderObjects(Object* object, const Matrix4& m_parent,
                                std::vector<AssetRenderRequest>& requests);
};

} // namespace Datamodel
} // namespace Engine