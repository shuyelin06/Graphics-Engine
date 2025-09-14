#include "SceneGraph.h"

#include <assert.h>
#include <string>

#include "rendering/ImGui.h"

namespace Engine {
namespace Datamodel {
Scene::Scene() : objects() {
    terrain = nullptr;
#if defined(_DEBUG)
    selected_object = nullptr;
#endif
}
Scene::~Scene() {
    for (Object* object : objects)
        delete object;
}

// ImGuiDisplayHierarchy
// Displays the object hierarchy in the "Scene" menu
// of the ImGui display
#if defined(_DEBUG)
static int displayObjectInfo(Object* object, int next_id, Object** active_obj) {
    const std::string name_unique =
        object->getClassName() + "##" + std::to_string(next_id++);
    const std::string button_unique =
        "Open Config##" + std::to_string(next_id++);

    ImGuiBackendFlags flags = 0;
    if (object->getChildren().empty())
        flags |= ImGuiTreeNodeFlags_Leaf;

    if (ImGui::TreeNodeEx(name_unique.c_str(), flags)) {
        if (ImGui::Button(button_unique.c_str())) {
            *active_obj = object;
        }

        for (Object* child : object->getChildren())
            next_id = displayObjectInfo(child, next_id, active_obj);

        ImGui::TreePop();
    }

    return next_id;
}

void Scene::imGuiDisplay() {
    if (ImGui::BeginMenu("Scene")) {
        // Display the scene hierarchy
        int next_id = 0;

        ImGui::SeparatorText("Scene Hierarchy");
        for (Object* object : objects)
            next_id = displayObjectInfo(object, next_id, &selected_object);

        // Display the active object config
        if (selected_object != nullptr) {
            ImGui::SeparatorText(selected_object->getClassName().c_str());
            ImGui::Separator();

            selected_object->propertyDisplay();
        }

        ImGui::EndMenu();
    }
}
#endif

// --- Object Handling ---
void Scene::addObject(Object* object) {
    assert(object->getParent() == nullptr);
    objects.push_back(object);
}

const std::vector<Object*>& Scene::getObjects() const { return objects; }

// --- Terrain Handling ---
void Scene::enableTerrain() { terrain = new Terrain(); }
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
static void updateObjectsHelper(Object* object, const Matrix4& m_parent) {
    assert(object != nullptr);

    const Matrix4 m_local = object->updateLocalMatrix(m_parent);
    std::vector<Object*> children = object->getChildren();

    std::vector<Object*>::iterator iter = children.begin();
    while (iter != children.end()) {
        if ((*iter)->shouldDestroy()) {
            delete *iter;
            iter = children.erase(iter);
        } else {
            updateObjectsHelper((*iter), m_local);
            iter++;
        }
    }
}

void Scene::updateAndCleanObjects() {
    Matrix4 identity = Matrix4::Identity();

    std::vector<Object*>::iterator iter = objects.begin();
    while (iter != objects.end()) {
        if ((*iter)->shouldDestroy()) {
            delete *iter;
            iter = objects.erase(iter);
        } else {
            updateObjectsHelper((*iter), identity);
            iter++;
        }
    }
}

} // namespace Datamodel
} // namespace Engine