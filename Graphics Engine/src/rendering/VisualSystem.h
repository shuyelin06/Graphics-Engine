#pragma once

#include <map>
#include <string>
#include <utility>
#include <vector>

#include "Direct3D11.h"

#include "core/ResourceManager.h"
#include "core/TextureManager.h"
#include "shaders/ShaderManager.h"
#include "lights/LightManager.h"

#include "VisualTerrain.h"
#include "core/AssetObject.h"
#include "lights/LightObject.h"

#include "rendering/core/Camera.h"

#if defined(_DEBUG)
// imgui Includes
#include "ImGui.h"

#include "util/GPUTimer.h"
#include "util/CPUTimer.h"
#endif

// TESTING
#include "core/TextureAtlas.h"
#include "math/Compute.h"

namespace Engine {
namespace Graphics {

// Render Information
struct ShadowCaster {
    Mesh* mesh;
    Matrix4 m_localToWorld;
};
struct RenderableTerrain {
    Mesh* mesh;
    Vector3 terrain_offset;
};

// VisualSystem Class:
// Provides an interface for the application's graphics.
class VisualSystem {
  private:
    // Window
    HWND window;

    // Direct 3D 11 Interfaces
    ID3D11Device* device;
    ID3D11DeviceContext* context;

    // Main Render Targets
    IDXGISwapChain* swap_chain;
    ID3D11RenderTargetView* render_target_view;
    D3D11_VIEWPORT viewport;

    // Managers
    ShaderManager* shaderManager;
    VisualResourceManager* assetManager;
    TextureManager* texture_manager;
    LightManager* light_manager;

    // Main Camera:
    // The scene is rendered from this camera
    Camera camera;

    // Render Information:
    // Vectors of rendering information that the visual system actually uses
    // for rendering. It takes the datamodel state, and processes them into
    // render information.
    std::vector<AssetObject*> renderable_assets;
    std::vector<ShadowLightObject*> shadow_lights;
    std::vector<VisualTerrain*> terrain_chunks;

    std::vector<ShadowCaster> shadow_casters;
    std::vector<RenderableTerrain> renderable_terrain;

  public:
    VisualSystem(HWND _window);

    // Returns the system's camera
    const Camera& getCamera() const;
    Camera& getCamera();

    // Initialize Visual System
    void initialize();
    // Shutdown Visual System
    void shutdown();

    // Create objects in the visual system
    AssetObject* bindAssetObject(Object* object, const std::string& asset_name);
    ShadowLightObject* bindShadowLightObject(Object* object);
    VisualTerrain* bindVisualTerrain(TerrainChunk* terrain);

    // Renders an entire scene
    void render();

  private:
    void renderPrepare(); // Prepare for Rendering

    void performShadowPass();  // Shadow Pass
    void performTerrainPass(); // Render Terrain
    void performRenderPass();  // Render Pass

    void renderFinish(); // Finish Rendering

    void renderDebugPoints(); // DEBUG
    void renderDebugLines();  // DEBUG

#if defined(_DEBUG) 
  private:
    // Debug via ImGui
    // Frametime Tracking (CPU + GPU)
    GPUTimer gpu_timer;
    CPUTimer cpu_timer;

    void imGuiInitialize();

    void imGuiPrepare();
    void imGuiFinish();

    void imGuiShutdown();

    // Debug via VisualDebug
    ID3D11Buffer* line_vbuffer;
#endif

};
} // namespace Graphics
} // namespace Engine