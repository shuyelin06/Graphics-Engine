#include "SceneGraph.h"

#include <assert.h>

namespace Engine {
namespace Datamodel {

Scene::Scene() : objects() { terrain_temp = new Terrain(); }
Scene::~Scene() {
    for (Object* object : objects)
        delete object;
}

// GetObjects:
// Returns the vector of objects in the SceneGraph
const std::vector<Object*>& Scene::getObjects() { return objects; }

// NewObject:
// Creates a new object in the scene.
Object& Scene::createObject() {
    Object* object = new Object();
    objects.push_back(object);
    return *object;
}

// --- Terrain Handling ---
const Terrain* Scene::getTerrain() const { return terrain_temp; }

// UpdateTerrainChunks:
// Updates the scene center and loads / unloads chunks based on this center.
void Scene::updateTerrainChunks(float x, float y, float z) {
    if (terrain_temp != nullptr)
        terrain_temp->reloadTerrain(x, y, z);
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