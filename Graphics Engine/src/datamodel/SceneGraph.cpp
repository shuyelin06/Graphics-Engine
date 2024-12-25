#include "SceneGraph.h"

#include <assert.h>

namespace Engine {
namespace Datamodel {

SceneGraph::SceneGraph() : objects() {
    for (int i = 0; i < CHUNK_X_LIMIT; i++) {
        for (int j = 0; j < CHUNK_Z_LIMIT; j++) {
            terrain_chunks[i][j] = new Terrain(i, j);
        }
    }
}
SceneGraph::~SceneGraph() {
    for (Object* object : objects)
        delete object;

    for (int i = 0; i < CHUNK_X_LIMIT; i++) {
        for (int j = 0; j < CHUNK_Z_LIMIT; j++) {
            delete terrain_chunks[i][j];
        }
    }
}

// GetObjects:
// Returns the vector of objects in the SceneGraph
const std::vector<Object*>& SceneGraph::getObjects() { return objects; }

// NewObject:
// Creates a new object in the scene.
Object& SceneGraph::createObject() {
    Object* object = new Object();
    objects.push_back(object);
    return *object;
}

// GetTerrain:
// Returns the scene's terrain.
const Terrain* SceneGraph::getTerrain(int x, int z) const {
    return terrain_chunks[x][z];
}
Terrain* SceneGraph::getTerrain(int x, int z) { return terrain_chunks[x][z]; }

// UpdateAndRenderTerrain:
// Update and submit render requests for the terrain chunks.
void SceneGraph::updateAndRenderTerrain(
    std::vector<TerrainRenderRequest>& requests) {
    for (int i = 0; i < CHUNK_X_LIMIT; i++) {
        for (int j = 0; j < CHUNK_Z_LIMIT; j++) {
            TerrainData terrain_data =
                TerrainData(terrain_chunks[i][j]->getRawData());
            TerrainRenderRequest request =
                TerrainRenderRequest(i, j, terrain_data);
            requests.push_back(request);
        }
    }
}

// UpdateAndRenderObjects:
// Update and cache object transforms in the SceneGraph, and submit
// render requests for each.
void SceneGraph::updateAndRenderObjects(
    std::vector<AssetRenderRequest>& requests) {
    Matrix4 identity = Matrix4::identity();

    for (Object* object : objects)
        updateAndRenderObjects(object, identity, requests);
}

void SceneGraph::updateAndRenderObjects(
    Object* object, const Matrix4& m_parent,
    std::vector<AssetRenderRequest>& requests) {
    assert(object != nullptr);

    const Matrix4 m_local = object->updateLocalMatrix(m_parent);

    if (object->getAsset() != NoAsset) {
        AssetRenderRequest request =
            AssetRenderRequest(object->getAsset(), m_local);
        requests.push_back(request);
    }

    for (Object* child : object->getChildren())
        updateAndRenderObjects(child, m_local, requests);
}

} // namespace Datamodel
} // namespace Engine