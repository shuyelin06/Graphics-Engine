#pragma once

#include <map>
#include <string>
#include <utility>
#include <vector>

#include "Direct3D11.h"

#include "AssetManager.h"
#include "RenderRequest.h"
#include "ShaderManager.h"

#include "rendering/components/Camera.h"
#include "rendering/components/Light.h"

#include "datamodel/Terrain.h"

namespace Engine {
namespace Graphics {

struct RenderCommand {
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
    ID3D11DepthStencilView* depth_stencil;

    // Managers
    ShaderManager* shaderManager;
    AssetManager* assetManager;

    // Main Camera:
    // The scene is rendered from this camera
    Camera camera;

    // Dynamic Lights:
    // Lights that are transformable in the scene
    std::vector<Light*> lights;
    Light* sun_light; // Always index 0 of lights

    // Render Requests:
    // Vectors of render requests submitted to the visual system.
    std::vector<AssetRenderRequest> assetRequests;
    std::vector<TerrainRenderRequest> terrainRequests;

    // Render Commands:
    // Vectors of rendering information that the visual system actually uses
    // for rendering. It takes all render requests, and processes them into these commands.
    std::vector<RenderCommand> renderCommands;

  public:
    VisualSystem(HWND _window);

    // Returns the system's camera
    const Camera& getCamera() const;
    Camera& getCamera();

    // Initialize Visual System
    void initialize();

    // Create objects in the visual system
    Light* createLight();
    Light* createLight(ShadowMapQuality quality);

    // Submit render requests to the visual system. These requests
    // are processed into render commands, which the system will use for rendering.
    void drawAsset(const AssetRenderRequest& renderRequest);
    void drawTerrain(const TerrainRenderRequest& renderRequest);

    // Renders an entire scene
    void render();

  private:                    // Rendering Helper Methods
    void renderPrepare();     // Prepare for Rendering
    void performShadowPass(); // Shadow Pass
    void performRenderPass(); // Render Pass
    void renderDebugPoints(); // DEBUG
    void renderDebugLines();  // DEBU

  public:
    // --- Data / Resource Queries ---
    // Get current viewport
    D3D11_VIEWPORT getViewport() const;

    // Create Texture
    ID3D11Texture2D* CreateTexture2D(D3D11_BIND_FLAG bind_flag, int width,
                                     int height);
};
} // namespace Graphics
} // namespace Engine