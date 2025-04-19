#include "SceneGraph.h"

#include <assert.h>

namespace Engine {
namespace Datamodel {

Scene::Scene() : objects() { terrain = new Terrain(); }
Scene::~Scene() {
    for (Object* object : objects)
        delete object;
}

// GetObjects:
// Returns the vector of objects in the SceneGraph
const std::vector<Object*>& Scene::getObjects() const { return objects; }

// NewObject:
// Creates a new object in the scene.
Object& Scene::createObject() {
    Object* object = new Object();
    objects.push_back(object);
    return *object;
}

// --- Terrain Handling ---
Terrain* Scene::getTerrain() const { return terrain; }

// InvalidateTerrainChunks:
// Given a position, invalidates the terrain chunks too far
// from this position, and places their positions in a priority queue
// to be reloaded.
void Scene::invalidateTerrainChunks(float x, float y, float z) {
    if (terrain != nullptr)
        terrain->invalidateTerrain(x, y, z);
}

// UpdateAndRenderObjects:
// Update and cache object transforms in the SceneGraph, and submit
// render requests for each.
void Scene::updateObjects() {
    Matrix4 identity = Matrix4::Identity();

    for (Object* object : objects)
        updateObjectsHelper(object, identity);
}

void Scene::updateObjectsHelper(Object* object, const Matrix4& m_parent) {
    assert(object != nullptr);

    const Matrix4 m_local = object->updateLocalMatrix(m_parent);
    for (Object* child : object->getChildren())
        updateObjectsHelper(child, m_local);
}

} // namespace Datamodel
} // namespace Engine