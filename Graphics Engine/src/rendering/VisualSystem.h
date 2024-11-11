#pragma once

#include <vector>
#include <map>
#include <string>
#include <utility>

#include "Direct3D11.h"

#include "AssetManager.h"
#include "ShaderManager.h"

#include "rendering/components/LightComponent.h"
#include "rendering/components/AssetComponent.h"

#include "rendering/components/Camera.h"

#include "datamodel/Terrain.h"

namespace Engine
{
namespace Graphics
{
	// VisualSystem Class:
	// Provides an interface for the application's graphics.
	class VisualSystem
	{
	private:
		// Window
		HWND window;
		IDXGISwapChain* swap_chain;

		// Direct 3D 11 Interfaces
		ID3D11Device* device;
		ID3D11DeviceContext* context;

		// Managers
		ShaderManager shaderManager;
		AssetManager assetManager;

		// Components
		std::vector<LightComponent*> lightComponents;
		std::vector<AssetComponent*> assetComponents;
		
		// Main Render Target
		ID3D11RenderTargetView* render_target_view;
		ID3D11DepthStencilView* depth_stencil;

        // Main Camera: The scene is rendered from this camera
        Camera camera;

	public:
		VisualSystem(HWND _window);
		
        // Returns the system's camera
        Camera& getCamera();

		// Initialize Visual System
		void initialize();

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
		ID3D11Texture2D* CreateTexture2D(D3D11_BIND_FLAG bind_flag, int width, int height);

		// Component Handling
		AssetComponent* bindAssetComponent(Datamodel::Object* object, AssetSlot assetName);
		bool removeAssetComponent(AssetComponent* component);

		LightComponent* bindLightComponent(Datamodel::Object* object);
		bool removeLightComponent(LightComponent* component);
	};
}
}