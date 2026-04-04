#pragma once

#include <cstdint>
#include <map>
#include <string>
#include <utility>
#include <vector>

#include "Direct3D11.h"
#include "ImGui.h"

#include "datamodel/SceneGraph.h"

#include "lights/LightManager.h"
#include "pipeline/PipelineManager.h"
#include "pipeline/RenderManager.h"
#include "terrain/TerrainManager.h"
#include "scene/SceneManager.h"

#include "core/RenderableMesh.h"

#include "rendering/core/Camera.h"

#if defined(_DEBUG)
#include "util/CPUTimer.h"
#include "util/GPUTimer.h"
#endif

#include "VisualDebug.h"

// TESTING
#include "core/TextureAtlas.h"
#include "math/Compute.h"

namespace Engine {
using namespace Datamodel;

namespace Graphics {
class SceneListener;
class SceneManager;
class ResourceManager;
class TerrainManager;

// VisualParameters Struct:
// Stores configuration parameters toggleable by the user
struct VisualParameters;
// VisualCache Struct:
// Stores precomputed data so that it can be reused many times in
// different parts of the pipeline
struct VisualCache;

// VisualSystem Class:
// Provides an interface for the application's graphics.
// VisualSystem is in charge of the different rendering passes;
// pipeline provides a convenient interface for some functionality.
class VisualSystem {
  private:
    // Frame
    uint64_t frame;

    // Configuration + Cache
    VisualParameters* config;
    VisualCache* cache;

    // Direct 3D 11 Interfaces
    ID3D11Device* device;
    ID3D11DeviceContext* context;

    // Managers
    std::unique_ptr<SceneListener> scene_listener;
    std::unique_ptr<SceneManager> scene_manager;
    std::unique_ptr<ResourceManager> resource_manager;
    std::unique_ptr<RenderManager> render_manager;
    LightManager* light_manager;
    Pipeline* pipeline;

    // Supported Components
    std::vector<RenderableMesh*> renderable_meshes;
    std::unique_ptr<TerrainManager> terrain;

    // Temp for now; should be moved later.
    ID3D11RasterizerState* og_rast_state;
    ID3D11RasterizerState* rast_state;
    Texture* bump_tex;

  public:
    VisualSystem(HWND window);

    // Datamodel Handling
    void onObjectCreate(Object* object);

    // Call these functions to render the scene. Renders an entire scene
    void renderPrepare();
    void pullSceneData(Scene* scene, Vector3 pos); // Call First
    void render();

    ResourceManager* getResourceManager() const;
    SceneListener* getSceneListener() const;
    SceneManager* getSceneManager() const;
    TerrainManager* getVisualTerrain() const;
    RenderManager* getRenderManager() const;
    LightManager* getLightManager() const;

    Pipeline* getPipeline() const;

  private:                     // Rendering Stages
    void performPrepass();     // Prepass (Shadowmaps)
    void performDefaultPass(); // Render Pass

    void performLightFrustumPass(); // Light Frustum Pass

    // Above Water Processing
    void performWaterSurfacePass(); // Water Surface Pass
    void processSky();
    void processUnderwater(); // Underwater Effect

  private:
    void imGui();

#if defined(ENABLE_DEBUG_DRAWING)
    // Debug via VisualDebug
    ID3D11Buffer* line_vbuffer;

    void renderDebugPoints(); // DEBUG
    void renderDebugLines();  // DEBUG
#endif
};
} // namespace Graphics
} // namespace Engine