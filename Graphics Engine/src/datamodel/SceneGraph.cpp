#include "SceneGraph.h"

#include <assert.h>
#include <string>

#include "rendering/ImGui.h"

namespace Engine {
namespace Datamodel {
Scene::Scene() : objects() {
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
#ifdef IMGUI_ENABLED
int Scene::imGuiTraverseHierarchy(Object* object, int next_id) {
    const std::string& object_name = object->getName();
    const std::string node_id = "##" + std::to_string(next_id++);

    ImGuiBackendFlags flags = 0;
    if (object->getChildren().empty())
        flags |= ImGuiTreeNodeFlags_Leaf;

    const bool dropdown_active = ImGui::TreeNodeEx(node_id.c_str(), flags);
    ImGui::SameLine();

    const bool properties_active = ImGui::Selectable(
        object_name.c_str(), false, ImGuiSelectableFlags_AllowItemOverlap);

    if (properties_active)
    {
        show_property_window = true;
        selected_object = object;
    }

    if (dropdown_active)
    {
        for (Object* child : object->getChildren())
            next_id = imGuiTraverseHierarchy(child, next_id);
        ImGui::TreePop();
    }

    return next_id;
}
#endif

void Scene::imGuiDisplay() {
#ifdef IMGUI_ENABLED
    if (ImGui::BeginMenu("Scene")) {
        // Display the scene hierarchy
        int next_id = 0;

        ImGui::SeparatorText("Scene Hierarchy");
        for (Object* object : objects)
            next_id = imGuiTraverseHierarchy(object, next_id);

        ImGui::EndMenu();
    }

    // Property Panel
    if (show_property_window && selected_object != nullptr) {
        if (ImGui::Begin("Property Window", &show_property_window,
                         ImGuiWindowFlags_NoCollapse)) {
            ImGui::SeparatorText(selected_object->getName().c_str());
            ImGui::Separator();

            selected_object->propertyDisplay();
        }
        ImGui::End();
    }
#endif
}

// --- Object Handling ---
void Scene::addObject(Object* object) {
    assert(object->getParent() == nullptr);
    objects.push_back(object);
}

const std::vector<Object*>& Scene::getObjects() const { return objects; }

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