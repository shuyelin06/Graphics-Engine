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
#include "terrain/VisualTerrain.h"

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

enum RenderTargetBindFlags {
    DisableDepthStencil,
    EnableDepthStencil_TestAndWrite,
    EnableDepthStencil_TestNoWrite
};

// Parameters for the VisualSystem
struct VisualParameters {
    Vector3 sun_direction;
    Vector3 sun_color;
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

    // Render target information. Lets us ping pong
    // between two render targets for post processing effects.
    Texture* render_target_dest; // We Render to This
    Texture* render_target_src;  // We Read from This

    Texture* depth_stencil;
    D3D11_VIEWPORT viewport;

    ID3D11BlendState* blend_state;

    // Managers
    ResourceManager* resource_manager;
    LightManager* light_manager;
    PipelineManager* pipeline_manager;

    // Render Information:
    // Rendering configurations
    float time_elapsed;
    float time_of_day;

    // Supported Components
    VisualParameters config;

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
    void initializeManagers();
    void initializeComponents();

  private:                     // Rendering Stages
    void performShadowPass();  // Shadow Pass
    void performTerrainPass(); // Render Terrain
    void performRenderPass();  // Render Pass

    void performLightFrustumPass(); // Light Frustum Pass

    // Above Water Processing
    void performWaterSurfacePass(); // Water Surface Pass
    void processSky();

    void processUnderwater(); // Underwater Effect

    void renderFinish(); // Finish Rendering

    // --- Rendering Helper Methods ---
    // Set the render targets for a given pass
    void bindActiveRenderTarget(RenderTargetBindFlags bind_flags);
    void swapActiveRenderTarget();

#if defined(_DEBUG)
  private:
    // Debug via ImGui
    // Frametime Tracking (CPU + GPU)
    GPUTimer gpu_timer;

    void imGuiInitialize(HWND window);

    void imGuiPrepare();
    void imGuiConfig();
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