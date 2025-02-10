#include "SceneGraph.h"

#include <assert.h>

namespace Engine {
namespace Datamodel {

SceneGraph::SceneGraph() : objects() {
    terrain = new Terrain(-100,-100);
}
SceneGraph::~SceneGraph() {
    for (Object* object : objects)
        delete object;
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

// UpdateAndRenderTerrain:
// Update the terrain chunks.
void SceneGraph::updateAndRenderTerrain() {
}

// UpdateAndRenderObjects:
// Update and cache object transforms in the SceneGraph, and submit
// render requests for each.
void SceneGraph::updateAndRenderObjects() {
    Matrix4 identity = Matrix4::identity();

    for (Object* object : objects)
        updateAndRenderObjects(object, identity);
}

void SceneGraph::updateAndRenderObjects(
    Object* object, const Matrix4& m_parent) {
    assert(object != nullptr);

    const Matrix4 m_local = object->updateLocalMatrix(m_parent);
    for (Object* child : object->getChildren())
        updateAndRenderObjects(child, m_local);
}

} // namespace Datamodel
} // namespace Engine