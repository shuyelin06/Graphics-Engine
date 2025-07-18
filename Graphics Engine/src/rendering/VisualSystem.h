#pragma once

#include <map>
#include <string>
#include <utility>
#include <vector>

#include "Direct3D11.h"
#include "ImGui.h"

#include "datamodel/SceneGraph.h"

#include "lights/LightManager.h"
#include "pipeline/PipelineManager.h"
#include "resources/ResourceManager.h"

#include "core/AssetComponent.h"
#include "datamodel/ComponentHandler.h"
#include "lights/LightComponent.h"
#include "terrain/VisualTerrain.h"

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

struct RenderableAsset {
    const Asset* asset;
    Matrix4 m_localToWorld;
};

// VisualParameters Struct:
// Stores configuration parameters toggleable by the user
struct VisualParameters;
// VisualCache Struct:
// Stores precomputed data so that it can be reused many times in
// different parts of the pipeline
struct VisualCache;

// VisualSystem Class:
// Provides an interface for the application's graphics.
class VisualSystem {
  private:
    // Configuration + Cache
    VisualParameters* config;
    VisualCache* cache;

    // Direct 3D 11 Interfaces
    ID3D11Device* device;
    ID3D11DeviceContext* context;

    ID3D11RasterizerState* og_rast_state;
    ID3D11RasterizerState* rast_state;

    Texture* bump_tex;

    // Managers
    ResourceManager* resource_manager;
    LightManager* light_manager;
    PipelineManager* pipeline;

    // Supported Components
    CameraComponent* camera;
    ComponentHandler<AssetComponent> asset_components;
    ComponentHandler<ShadowLightComponent> light_components;
    VisualTerrain* terrain;

    std::vector<Mesh*> visible_chunks;
    std::vector<RenderableAsset> renderable_meshes;

  public:
    VisualSystem(HWND window);

    // Visual System Bindings
    void registerComponents();
    void bindComponents(const std::vector<ComponentBindRequest>& requests);

    AssetComponent* bindAssetComponent(Object* object,
                                       const std::string& asset_name);

    // Call these functions to render the scene. Renders an entire scene
    void pullSceneData(Scene* scene); // Call First
    void render();

  private:                     // Rendering Stages
    void performPrepass();     // Prepass (Shadowmaps)
    void performTerrainPass(); // Render Terrain
    void performRenderPass();  // Render Pass

    void performLightFrustumPass(); // Light Frustum Pass

    // Above Water Processing
    void performWaterSurfacePass(); // Water Surface Pass
    void processSky();
    void processUnderwater(); // Underwater Effect

#if defined(_DEBUG)
  private:
    void imGuiConfig();
#endif

#if defined(ENABLE_DEBUG_DRAWING)
    // Debug via VisualDebug
    ID3D11Buffer* line_vbuffer;

    void renderDebugPoints(); // DEBUG
    void renderDebugLines();  // DEBUG
#endif
};
} // namespace Graphics
} // namespace Engine