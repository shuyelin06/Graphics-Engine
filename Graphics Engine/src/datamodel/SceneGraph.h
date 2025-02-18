#pragma once

#include <vector>

#include "Object.h"
#include "Terrain.h"

// Stores the number of terrain chunks past the center chunk we will maintain.
/*
               /\ Extent
                |
                |
              -----
             |     |
  Extent<--  |     | --> Extent
              -----
                |
                |
               \/ Extent

*/
constexpr int TERRAIN_CHUNK_EXTENT = 3;
constexpr int TERRAIN_NUM_CHUNKS = 2 * TERRAIN_CHUNK_EXTENT + 1;

namespace Engine {
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

  public:
    // Center chunk of the scene. Loading is performed based on this center
    int center_chunk_x, center_chunk_z;
    // Loaded terrain chunks in the scene
    TerrainChunk* terrain[TERRAIN_NUM_CHUNKS][TERRAIN_NUM_CHUNKS];
    // Temporarily stores terrain chunks for when the scene center changes
    TerrainChunk* terrain_helper[TERRAIN_NUM_CHUNKS][TERRAIN_NUM_CHUNKS];

  public:
    Scene();
    ~Scene();

    // Object handling
    const std::vector<Object*>& getObjects();
    Object& createObject();

    // Update the center of the scene graph. Based on the center, the scene
    // graph will generate terrain chunks
    void updateSceneCenter(float center_x, float center_z);

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