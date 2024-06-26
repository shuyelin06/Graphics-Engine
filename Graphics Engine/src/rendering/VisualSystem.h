#pragma once

#include <vector>
#include <map>
#include <string>
#include <utility>

#include "Direct3D11.h"

#include "datamodel/Subsystem.h"
#include "datamodel/ComponentHandler.h"

#include "rendering/components/LightComponent.h"
#include "rendering/components/ViewComponent.h"
#include "rendering/components/MeshComponent.h"

#include "ShaderData.h"

namespace Engine
{
using namespace Datamodel;

namespace Graphics
{
	// Shader_Type Enum:
	// Represents shader types in a more readable
	// format, for internal use
	typedef enum { Vertex, Pixel } Shader_Type;

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
	// Provides an interface for the application's graphics
	class VisualSystem
		:
		public Datamodel::Subsystem,
		public Datamodel::ComponentHandler<ViewComponent>,
		public Datamodel::ComponentHandler<MeshComponent>,
		public Datamodel::ComponentHandler<LightComponent>
	{
	private:
		// Window Handle
		HWND window;

		// Direct 3D 11 Pointers
		ID3D11Device* device;
		ID3D11DeviceContext* device_context;
		IDXGISwapChain* swap_chain;
		
		// Rendering
		ID3D11RenderTargetView* render_target_view;
		ID3D11DepthStencilView* depth_stencil;

		// Available Constant Buffers
		std::vector<ID3D11Buffer*> vs_constant_buffers;
		std::vector<ID3D11Buffer*> ps_constant_buffers;

		// Available Resources
		std::map<char, ID3D11InputLayout*> input_layouts;
		std::map<std::string, ID3D11VertexShader*> vertex_shaders;
		std::map<std::string, ID3D11PixelShader*> pixel_shaders;

		// Mesh Index/Vertex Buffer Cache
		std::map<Mesh*, MeshBuffers> mesh_cache;

	public:
		VisualSystem(HWND _window);
		
		// Initialize Visual System
		void initialize();

		// Renders an Entire Scene
		void update();

		// --- Data / Resource Queries ---
		// Direct3D Resource / Interface Queries
		ID3D11Device* getDevice() const;
		ID3D11DeviceContext* getDeviceContext();
		ID3D11RenderTargetView* getRenderTargetView() const;
		ID3D11DepthStencilView* getDepthStencil() const;

		ID3D11InputLayout* getInputLayout(char layout) const;
		ID3D11VertexShader* getVertexShader(std::string name) const;
		ID3D11PixelShader* getPixelShader(std::string name) const;

		// Get current viewport
		D3D11_VIEWPORT getViewport() const;

		// Returns the currently active view
		ViewComponent* getActiveView();

		// Creates / returns the index and vertex buffers for a mesh.
		// Maintains the options to cache the buffers for later use.
		MeshBuffers getMeshBuffers(Mesh* mesh, bool cache);
		
		// --- Resource Creation ---
		// Create Buffers
		ID3D11Buffer* CreateBuffer(D3D11_BIND_FLAG, void *data, int byte_size);

		// Create Texture
		ID3D11Texture2D* CreateTexture2D(D3D11_BIND_FLAG bind_flag, int width, int height);

		// Compile and Create Shaders
		ID3D11VertexShader* CreateVertexShader(const wchar_t* file, const char* entry, char layout);
		ID3D11PixelShader* CreatePixelShader(const wchar_t* file, const char* entry);

		// --- Render Binding ---
		// Bind Data to Constant Buffers
		void BindVSData(unsigned int index, void* data, int byte_size);
		void BindPSData(unsigned int index, void* data, int byte_size);
		

	private:
		void BindData(Shader_Type type, unsigned int index, void* data, int byte_size);
	};
}
}