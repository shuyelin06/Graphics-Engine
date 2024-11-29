#include "SceneGraph.h"

namespace Engine {
namespace Datamodel {

SceneGraph::SceneGraph() = default;
SceneGraph::~SceneGraph() {
    for (Object* object : objects)
        delete object;

    delete terrain;
}

Object* SceneGraph::newObject() {
    Object* object = new Object();
    objects.push_back(object);
    return object;
}

} // namespace Datamodel
} // namespace Engine