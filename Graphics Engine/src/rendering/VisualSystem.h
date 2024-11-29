#pragma once

#include <map>
#include <string>
#include <utility>
#include <vector>

#include "Direct3D11.h"

#include "AssetManager.h"
#include "ShaderManager.h"

#include "rendering/components/Camera.h"
#include "rendering/components/Light.h"

#include "datamodel/Terrain.h"

namespace Engine {
namespace Graphics {
// RenderRequest Class(es):
// Structures that represent render requests that are submitted
// to the visual system.
struct AssetRenderRequest {
    AssetSlot slot;
    Matrix4 mLocalToWorld;

    AssetRenderRequest(AssetSlot slot, const Matrix4& mLocalToWorld);
};

// VisualSystem Class:
// Provides an interface for the application's graphics.
class VisualSystem {
  private:
    // Window
    HWND window;
    IDXGISwapChain* swap_chain;

    // Direct 3D 11 Interfaces
    ID3D11Device* device;
    ID3D11DeviceContext* context;

    // Managers
    ShaderManager shaderManager;
    AssetManager* assetManager;

    // Main Render Target
    ID3D11RenderTargetView* render_target_view;
    ID3D11DepthStencilView* depth_stencil;

    // Main Camera:
    // The scene is rendered from this camera
    Camera camera;

    // Dynamic Lights:
    // Lights that are transformable in the scene
    std::vector<Light*> lights;

    // Render Requests:
    // Vectors of render requests submitted to the visual system.
    std::vector<AssetRenderRequest> assetRequests;

  public:
    VisualSystem(HWND _window);

    // Returns the system's camera
    const Camera& getCamera() const;
    Camera& getCamera();

    // Initialize Visual System
    void initialize();

    // Create objects in the visual system
    Light* createLight();

    // Submit render requests to the visual system
    void drawAsset(AssetSlot asset, const Matrix4& mLocalToWorld);

    // Renders an entire scene
    void render();

  private:
    // Rendering helper methods
    void performShadowPass();
    void performRenderPass();
    void renderDebugPoints();
    void renderDebugLines();

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