#include "SceneGraph.h"

#include <assert.h>

namespace Engine {
namespace Datamodel {

SceneGraph::SceneGraph() : objects() { terrain = new Terrain(); }
SceneGraph::~SceneGraph() {
    for (Object* object : objects)
        delete object;

    delete terrain;
}

// GetObjects:
// Returns the vector of objects in the SceneGraph
const std::vector<Object*>& SceneGraph::getObjects() {
    return objects;
}

// NewObject:
// Creates a new object in the scene.
Object& SceneGraph::createObject() {
    Object* object = new Object();
    objects.push_back(object);
    return *object;
}

// GetTerrain:
// Returns the scene's terrain.
const Terrain* SceneGraph::getTerrain() const { return terrain; }
Terrain* SceneGraph::getTerrain() { return terrain; }

// UpdateObjectTransforms:
// Update and cache object transforms in the SceneGraph
void SceneGraph::updateObjectTransforms() {
    Matrix4 identity = Matrix4::identity();

    for (Object* object : objects)
        updateObjectTransforms(object, identity);
}

void SceneGraph::updateObjectTransforms(Object* object,
                                        const Matrix4& m_parent) {
    assert(object != nullptr);

    Matrix4 m_local = object->updateLocalMatrix(m_parent);

    for (Object* child : object->getChildren())
        updateObjectTransforms(child, m_local);
}

} // namespace Datamodel
} // namespace Engine