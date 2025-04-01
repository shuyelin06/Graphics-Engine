#pragma once

#include <map>
#include <string>
#include <utility>
#include <vector>

#include "Direct3D11.h"

#include "lights/LightManager.h"
#include "resources/ResourceManager.h"
#include "shaders/ShaderManager.h"
#include "shaders/PipelineManager.h"

#include "VisualTerrain.h"
#include "core/AssetObject.h"
#include "lights/LightObject.h"

#include "rendering/core/Camera.h"

#if defined(_DEBUG)
// imgui Includes
#include "ImGui.h"

#include "util/CPUTimer.h"
#include "util/GPUTimer.h"
#endif

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
struct RenderableMesh {
    const Mesh* mesh;
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

    // Main Camera:
    // The scene is rendered from this camera
    Camera camera;

    // Render Information:
    // Rendering configurations
    float time_of_day;

    // Vectors of rendering information that the visual system actually uses
    // for rendering. It takes the datamodel state, and processes them into
    // render information.
    std::vector<AssetObject*> renderable_assets;
    std::vector<ShadowLightObject*> shadow_lights;
    std::vector<VisualTerrain*> terrain_chunks;

    std::vector<RenderableMesh> renderable_meshes;
    std::vector<Mesh*> terrain_meshes;

  public:
    VisualSystem();

    // Returns the system's camera
    const Camera& getCamera() const;
    Camera& getCamera();

    // Initialize Visual System
    void initialize(HWND window);

    // Renders an entire scene
    void render();

    // Shutdown Visual System
    void shutdown();

    // Create objects in the visual system
    AssetObject* bindAssetObject(Object* object, const std::string& asset_name);
    ShadowLightObject* bindShadowLightObject(Object* object);
    VisualTerrain* bindVisualTerrain(TerrainChunk* terrain);

  private:
    // Initialization Stages of the Visual System
    void initializeScreenTarget(HWND window, UINT width, UINT height);
    void initializeRenderTarget(UINT width, UINT height);

    void initializeFullscreenQuad();

    void initializeManagers();

  private:
    // Rendering Stages of the Visual System
    void renderPrepare(); // Prepare for Rendering

    void performShadowPass();  // Shadow Pass
    void performTerrainPass(); // Render Terrain
    void performRenderPass();  // Render Pass

    void processSky(); // Blur Effect

    void renderFinish(); // Finish Rendering

    void renderDebugPoints(); // DEBUG
    void renderDebugLines();  // DEBUG

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

    // Debug via VisualDebug
    ID3D11Buffer* line_vbuffer;
#endif
};
} // namespace Graphics
} // namespace Engine