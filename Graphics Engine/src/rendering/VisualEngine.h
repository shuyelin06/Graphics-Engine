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
	class VisualEngine
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
		vector<ID3D11Buffer*> vs_constant_buffers;
		vector<ID3D11Buffer*> ps_constant_buffers;

		// Available Shaders
		vector<pair<ID3D11VertexShader*, ID3D11InputLayout*>> vertex_shaders; // Vertex Shader and Associated Input Layout
		vector<ID3D11PixelShader*> pixel_shaders; // Pixel Shaders
	
		// Mesh Cache
		std::map<std::string, Mesh> meshes;

		// Mesh Index/Vertex Buffer Cache
		std::map<Mesh*, MeshBuffers> mesh_cache;

	public:
		VisualEngine();
		void initialize(HWND _window); // Initialize Direct 3D
		
		// Rendering Methods
		void prepare(); // Prepares for a draw call
		
		void render(Scene* scene); // Render a Scene

		// Bind data to the vertex and pixel shaders
		void bind_vs_data(unsigned int index, void* data, int byte_size);
		void bind_ps_data(unsigned int index, void* data, int byte_size);

		// Draws an object from the current player's point of view
		void drawObject(Camera* camera, Object* object);

		void present(); // Present Drawn Content to Screen

	private:
		// Create Buffers
		ID3D11Buffer* create_buffer(D3D11_BIND_FLAG, void *data, int byte_size);

		// Helper Bind Data Method
		void bind_data(Shader_Type type, unsigned int index, void* data, int byte_size);

		// Compile and Create Shaders
		void create_vertex_shader(const wchar_t* file, const char* entry, D3D11_INPUT_ELEMENT_DESC[], int desc_size);
		void create_pixel_shader(const wchar_t* file, const char* entry);
		
		// Transformation Matrices - Defined in Transform.cpp
		static Matrix4 localToWorldMatrix(const Object* object);
		static Matrix4 projectionMatrix(const Camera* camera);

		static Matrix4 scaleMatrix(const Vector3 scale);
		static Matrix4 rotationMatrix(const Vector3 rotation);
		static Matrix4 translationMatrix(const Vector3 translation);
	};
}
}