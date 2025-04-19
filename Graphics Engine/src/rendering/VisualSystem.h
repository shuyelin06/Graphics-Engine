#pragma once

#include <map>
#include <string>
#include <utility>
#include <vector>

#include "Direct3D11.h"

#include "lights/LightManager.h"
#include "resources/ResourceManager.h"
#include "shaders/PipelineManager.h"
#include "shaders/ShaderManager.h"

#include "core/AssetComponent.h"
#include "datamodel/ComponentHandler.h"
#include "lights/LightComponent.h"

#include "VisualTerrain.h"

#include "rendering/core/Camera.h"

#if defined(_DEBUG)
// imgui Includes
#include "ImGui.h"

#include "util/CPUTimer.h"
#include "util/GPUTimer.h"
#endif
#include "VisualDebug.h"

// TESTING
#include "core/TextureAtlas.h"
#include "math/Compute.h"

namespace Engine {
namespace Graphics {

// Render Information
struct RenderableTerrain {
    Mesh* mesh;
    Vector3 terrain_offset;
};
struct RenderableAsset {
    const Asset* asset;
    Matrix4 m_localToWorld;
};

// VisualSystem Class:
// Provides an interface for the application's graphics.
class VisualSystem {
  private:
    // Direct 3D 11 Interfaces
    ID3D11Device* device;
    ID3D11DeviceContext* context;

    // Screen render target information
    IDXGISwapChain* swap_chain;
    Texture* screen_target;

    // Main render target information
    // We render to this and apply post-processing before moving
    // everything to the screen
    Texture* render_target;
    Texture* depth_stencil;

    ID3D11Buffer* postprocess_quad;
    D3D11_VIEWPORT viewport;

    // Managers
    ResourceManager* resource_manager;
    LightManager* light_manager;
    PipelineManager* pipeline_manager;

    // Render Information:
    // Rendering configurations
    float time_of_day;

    // Supported Components
    CameraComponent* camera;
    ComponentHandler<AssetComponent> asset_components;
    ComponentHandler<ShadowLightComponent> light_components;
    VisualTerrain* terrain;

    std::vector<RenderableAsset> renderable_meshes;

  public:
    VisualSystem(HWND window);

    // Renders an entire scene
    void pullDatamodelData(); // Call First
    void render();

    // Shutdown Visual System
    void shutdown();

  public: // Visual System Bindings
    CameraComponent* bindCameraComponent(Object* object);
    AssetComponent* bindAssetComponent(Object* object,
                                       const std::string& asset_name);
    ShadowLightComponent* bindLightComponent(Object* object);
    VisualTerrain* bindTerrain(Terrain* terrain);

  private: // Initialization Stages
    void initializeScreenTarget(HWND window, UINT width, UINT height);
    void initializeRenderTarget(UINT width, UINT height);
    void initializeFullscreenQuad();
    void initializeManagers();
    void initializeComponents();

  private:                     // Rendering Stages
    void performShadowPass();  // Shadow Pass
    void performTerrainPass(); // Render Terrain
    void performRenderPass();  // Render Pass
    void processUnderwater();  // Blur Effect
    void renderFinish();       // Finish Rendering

#if defined(_DEBUG)
  private:
    // Debug via ImGui
    // Frametime Tracking (CPU + GPU)
    GPUTimer gpu_timer;
    CPUTimer cpu_timer;

    void imGuiInitialize(HWND window);

    void imGuiPrepare();
    void imGuiFinish();

    void imGuiShutdown();
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