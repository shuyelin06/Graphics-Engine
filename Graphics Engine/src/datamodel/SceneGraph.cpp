#include "SceneGraph.h"

#include <assert.h>

#include "rendering/ImGui.h"

namespace Engine {
namespace Datamodel {
ComponentBindRequest::ComponentBindRequest(Object* o, unsigned int id) {
    target_object = o;
    component_id = id;
}

Scene::Scene() : objects(), visual_component_requests() {
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

            constexpr int MAX_NAME_LENGTH = 20;
            static char component_name[MAX_NAME_LENGTH];
            ImGui::InputText("Bind New Component", component_name,
                             MAX_NAME_LENGTH);
            ImGui::SameLine();
            if (ImGui::Button("Bind")) {
                bindComponent(*selected_object, component_name);
            }

            ImGui::Separator();

            for (Component* component : selected_object->getComponents()) {
                component->imGuiConfig();
            }
        }

        ImGui::EndMenu();
    }
}
#endif

// --- Scene Graph Queries ---
// QueryForClassID:
// Runs a brute force search and populates the vector with all object
// instances with this class ID.
// TODO: Could be optimized to only need 1 search every frame
static void queryForClassIDHelper(Object* object, uint16_t class_id,
                                  std::vector<Object*>& output) {
    if (object->getClassID() == class_id)
        output.push_back(static_cast<Object*>(object));

    for (Object* object : object->getChildren())
        queryForClassIDHelper(object, class_id, output);
}

void Scene::queryForClassID(const std::string& class_name,
                            std::vector<Object*>& output) const {
    const uint16_t id = Object::GetObjectClassIDByName(class_name);

    for (Object* object : objects)
        queryForClassIDHelper(object, id, output);
}

// --- Object Handling ---
void Scene::addObject(Object* object) {
    assert(object->getParent() == nullptr);
    objects.push_back(object);
}

// BindComponent:
// Submits a request to the scenegraph to bind a component to
// a given object.
// TODO: Only compatible with visual objects
void Scene::bindComponent(Object& object, const std::string& component_name) {
    const unsigned int id = Component::getTag(component_name);

    if (id != COMPONENT_TAG_NONE)
        visual_component_requests.push_back(ComponentBindRequest(&object, id));
}

// ClearVisualComponentRequests:
// Clears the visual component request vector.
void Scene::clearVisualComponentRequests() {
    visual_component_requests.clear();
}

const std::vector<Object*>& Scene::getObjects() const { return objects; }
const std::vector<ComponentBindRequest>&
Scene::getVisualComponentRequests() const {
    return visual_component_requests;
}

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