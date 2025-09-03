#pragma once

#include <vector>

#include "Object.h"
#include "terrain/Terrain.h"

namespace Engine {
using namespace Math;

namespace Datamodel {
// Struct ComponentBindRequest:
// Holds request data to bind a component to an object. This is the primary
// way the SceneGraph stays in sync with the other engine systems.
struct ComponentBindRequest {
    Object* target_object;
    unsigned int component_id;

    ComponentBindRequest(Object* o, unsigned int id);
};

// Class SceneGraph:
// Stores and manages all objects in the scene. Objects are stored in a
// tree-like hierarchy, Parent <--> Children Where all children node transforms
// are based off the local coordinate system of their parent. The reference
// coordinate system of all nodes without a parent is the world coordinate
// system.
class Scene {
  private:
    std::vector<Object*> objects;
    Terrain* terrain;

    std::vector<ComponentBindRequest> visual_component_requests;

#if defined(_DEBUG)
    Object* selected_object;
#endif(_DEBUG)

  public:
    Scene();
    ~Scene();

#if defined(_DEBUG)
    // Displays the object hierarchy in the "Scene" menu
    // of the ImGui display.
    void imGuiDisplay();
#endif(_DEBUG)

    // --- Scene Graph Queries ---
    void queryForClassID(const std::string& class_name,
                         std::vector<Object*>& output) const;

    // --- Object Handling ---
    void addObject(Object* object);
    void bindComponent(Object& object, const std::string& component_name);

    void clearVisualComponentRequests();

    const std::vector<Object*>& getObjects() const;
    const std::vector<ComponentBindRequest>& getVisualComponentRequests() const;

    // Update object transforms and submit render requests
    void updateObjects();

    // --- Terrain Handling ---
    void enableTerrain();

    Terrain* getTerrain() const;

    // Invalidate terrain chunks outside of our given position.
    void invalidateTerrainChunks(float x, float y, float z);

  private:
    void updateObjectsHelper(Object* object, const Matrix4& m_parent);
};

} // namespace Datamodel
} // namespace Engine