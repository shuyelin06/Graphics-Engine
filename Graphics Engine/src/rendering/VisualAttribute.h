#pragma once

#include "Direct3D11.h"

#include "datamodel/Scene.h"
#include "datamodel/Object.h"
#include "datamodel/Camera.h"

#include <map>
#include <vector>
#include <utility>

namespace Engine::Datamodel { class Scene; class Object; class Camera; }

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

	// VisualEngine Class:
	// Provides an interface for the application's graphics
	class VisualAttribute
	{
	/* Visual Attribute Engine 
		Static class that contains all of the Direct3D code necessary
		for rendering in the program. */
	// Static Class Variables
	protected:
		// Handle to Window
		static HWND window;

		// Direct 3D 11 Pointers
		static ID3D11Device* device;
		static ID3D11DeviceContext* device_context;
		static IDXGISwapChain* swap_chain;
		
		// Rendering Views
		static ID3D11RenderTargetView* render_target_view;
		static ID3D11DepthStencilView* depth_stencil;

		// Available Constant Buffers
		static vector<ID3D11Buffer*> vs_buffers;
		static vector<ID3D11Buffer*> ps_buffers;

		// Available Shaders
		static vector<pair<ID3D11VertexShader*, ID3D11InputLayout*>> vertex_shaders; // Vertex Shader and Associated Input Layout
		static vector<ID3D11PixelShader*> pixel_shaders; // Pixel Shaders
	
		// Visual Attributes
		static std::vector<VisualAttribute*> attributes;		

		// Camera to render from
		static Camera* camera;

	// Public Static Class Methods
	// Callable by people wanting to render a scene
	public:
		// Initialize Direct 3D and Other Visual Attribute Properties
		static void Initialize(HWND _window);

		// Render All Subscribed Visual Attributes from a Camera
		static void RenderAll();
		
		// TEMP: Set Camera
		static void SetCamera(Camera* _camera);

	// Protected Static Class Methods
	// Callable by the VisualAttributes
	protected:
		// Get an accessor to access object fields
		static ObjectAccessor GetObjectAccessor(void) { return ObjectAccessor(); };

		// Create Resources
		static void create_vertex_shader(const wchar_t* file, const char* entry, D3D11_INPUT_ELEMENT_DESC[], int desc_size);
		static void create_pixel_shader(const wchar_t* file, const char* entry);
		static ID3D11Buffer* create_buffer(D3D11_BIND_FLAG, void* data, int byte_size);

		// Bind data to the vertex and pixel shaders
		static void bind_vs_data(unsigned int index, void* data, int byte_size);
		static void bind_ps_data(unsigned int index, void* data, int byte_size);

	// Private Static Class Methods
	// Helper methods to accomplish the above tasks
	private:
		// Binds Data to a Shader
		static void bind_data(Shader_Type type, unsigned int index, void* data, int byte_size);

	// VisualAttribute Instance
	protected:
		Object* object;

	public:
		VisualAttribute(Object* object);
		~VisualAttribute();

		// Prepare for a draw call
		virtual void prepare(void) = 0;

		// Render an entire scene (call once per frame)
		virtual void render(void) = 0;

		// Finish drawing
		virtual void finish(void) = 0;
	};
}
}