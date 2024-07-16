#pragma once

#include <vector>
#include <map>
#include <string>
#include <utility>

#include "Direct3D11.h"

#include "AssetManager.h"
#include "ShaderManager.h"

#include "rendering/components/LightComponent.h"
#include "rendering/components/ViewComponent.h"
#include "rendering/components/AssetComponent.h"

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
		std::vector<LightComponent*> light_components;
		std::vector<AssetComponent*> asset_components;
		std::vector<ViewComponent*> view_components;
		
		// Main Render Target
		ID3D11RenderTargetView* render_target_view;
		ID3D11DepthStencilView* depth_stencil;

	public:
		VisualSystem(HWND _window);
		
		// Initialize Visual System
		void initialize();

		// Renders an entire scene
		void update();

		// --- Data / Resource Queries ---
		// Get current viewport
		D3D11_VIEWPORT getViewport() const;

		// Returns the currently active view
		ViewComponent* getActiveView();

		// Create Texture
		ID3D11Texture2D* CreateTexture2D(D3D11_BIND_FLAG bind_flag, int width, int height);

		// Component Handling
		AssetComponent* bindAssetComponent(Datamodel::Object* object, AssetSlot assetName);
		bool removeAssetComponent(AssetComponent* component);

		ViewComponent* bindViewComponent(Datamodel::Object* object);
		bool removeViewComponent(ViewComponent* component);

		LightComponent* bindLightComponent(Datamodel::Object* object);
		bool removeLightComponent(LightComponent* component);
	};
}
}