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
	// MeshBuffers Struct:
	// Stores pointers to D3D11 Index/Vertex Buffers, which are mapped to 
	// Mesh pointers. Used to cache Index/Vertex Buffers, to avoid
	// redundantly recreating resources
	struct MeshBuffers
	{
		ID3D11Buffer* vertex_buffer;
		ID3D11Buffer* index_buffer;
	};

	// VisualSystem Class:
	// Provides an interface for the application's graphics.
	// When rendering, the convention is to use the constant buffers
	// as follows, based on update frequency:
	// 1) Slot 1: Per-Frame Data
	// 2) Slot 2: Per-View Data 
	// 3) Slot 3: Per-Asset Data 
	// 3) Slot 4: Per-Mesh
	class VisualSystem
	{
	private:
		// Window Handle
		HWND window;

		// Direct 3D 11 Pointers
		ID3D11Device* device;
		ID3D11DeviceContext* device_context;
		IDXGISwapChain* swap_chain;

		// Components
		std::vector<LightComponent*> light_components;
		std::vector<AssetComponent*> asset_components;
		std::vector<ViewComponent*> view_components;

		

		
		
		// Rendering
		ID3D11RenderTargetView* render_target_view;
		ID3D11DepthStencilView* depth_stencil;

		// Managers
		ShaderManager shaderManager;
		AssetManager assetManager;
		
		std::map<Mesh*, MeshBuffers> mesh_cache;

	public:
		VisualSystem(HWND _window);
		
		// Initialize Visual System
		void initialize();

		// Renders an entire scene
		void update();

		// --- Data / Resource Queries ---
		// Direct3D Resource / Interface Queries
		ID3D11Device* getDevice() const;
		ID3D11DeviceContext* getDeviceContext();
		ID3D11RenderTargetView* getRenderTargetView() const;
		ID3D11DepthStencilView* getDepthStencil() const;

		// Get current viewport
		D3D11_VIEWPORT getViewport() const;

		// Returns the currently active view
		ViewComponent* getActiveView();

		// Create Texture
		ID3D11Texture2D* CreateTexture2D(D3D11_BIND_FLAG bind_flag, int width, int height);

		// --- Component Handling ---
		AssetComponent* bindAssetComponent(Datamodel::Object* object, std::string assetName);
		bool removeAssetComponent(AssetComponent* component);

		ViewComponent* bindViewComponent(Datamodel::Object* object);
		bool removeViewComponent(ViewComponent* component);

		LightComponent* bindLightComponent(Datamodel::Object* object);
		bool removeLightComponent(LightComponent* component);
	};
}
}