#pragma once

#include <map>
#include <string>
#include <utility>
#include <vector>

#include "Direct3D11.h"

#include "ResourceManager.h"
#include "RenderRequest.h"
#include "ShaderManager.h"
#include "TextureManager.h"
#include "core/LightManager.h"

#include "rendering/core/Camera.h"

#include "datamodel/Terrain.h"

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
struct RenderableAsset {
    Asset* asset;
    Matrix4 m_localToWorld;
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
    ResourceManager* assetManager;
    TextureManager* texture_manager;
    LightManager* light_manager;

    // Main Camera:
    // The scene is rendered from this camera
    Camera camera;

    // Render Requests:
    // Vectors of render requests submitted to the visual system.
    std::vector<AssetRenderRequest> assetRequests;
    std::vector<TerrainRenderRequest> terrainRequests;

    // Render Information:
    // Vectors of rendering information that the visual system actually uses
    // for rendering. It takes all render requests, and processes them into
    // this information.
    std::vector<ShadowCaster> shadow_casters;

    std::vector<RenderableTerrain> renderable_terrain;
    std::vector<RenderableAsset> renderable_assets;

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
    ShadowLight* createLight();
    ShadowLight* createLight(ShadowMapQuality quality);

    // Submit render requests to the visual system. These requests
    // are processed into render commands, which the system will use for
    // rendering.
    void drawAsset(const AssetRenderRequest& renderRequest);
    void drawTerrain(const TerrainRenderRequest& renderRequest);

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

#if defined(_DEBUG) // ImGui
  private:
    // Frametime Tracking (CPU + GPU)
    GPUTimer gpu_timer;
    CPUTimer cpu_timer;

    void imGuiInitialize();

    void imGuiPrepare();
    void imGuiFinish();

    void imGuiShutdown();
#endif

};
} // namespace Graphics
} // namespace Engine