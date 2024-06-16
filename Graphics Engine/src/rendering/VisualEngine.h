#pragma once

#include "Direct3D11.h"

#include "datamodel/Scene.h"
#include "datamodel/Object.h"
#include "datamodel/Camera.h"

#include "ShaderData.h"

#include <map>
#include <vector>

#include <string>

#include <utility>

namespace Engine::Datamodel { class Scene; class Object; class Camera; }

using namespace std;

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

		// Available Resources
		map<char, ID3D11InputLayout*> input_layouts;

		map<string, ID3D11VertexShader*> vertex_shaders;
		map<string, ID3D11PixelShader*> pixel_shaders;
		
		// Mesh Cache
		std::map<std::string, Mesh> meshes;

		// Mesh Index/Vertex Buffer Cache
		std::map<Mesh*, MeshBuffers> mesh_cache;

	public:
		VisualEngine();
		
		// Initialize Visual Engine
		void initialize(HWND _window);
		
		// Renders an entire scene
		void render(Scene& scene);

	private:
		// --- Debug Rendering ---
		void debugDrawPoints();
		void debugDrawLines();

		// --- Other ---

		// Traverse a SceneGraph for rendering
		void traverseSceneGraph(Scene& scene, Object* object, Matrix4& m_parent);

		// Renders a mesh in a scene, give a Local -> World transformation
		void renderMesh(Mesh& mesh, Matrix4& m_transform, Scene& scene, std::string shader_config, bool instancing );

		
		

		// Create Buffers
		ID3D11Buffer* create_buffer(D3D11_BIND_FLAG, void *data, int byte_size);

		// Create Texture
		ID3D11Texture2D* CreateTexture2D(D3D11_BIND_FLAG bind_flag, int width, int height);

		// Bind Data to Constant Buffers
		void BindVSData(unsigned int index, void* data, int byte_size);
		void BindPSData(unsigned int index, void* data, int byte_size);
		void BindData(Shader_Type type, unsigned int index, void* data, int byte_size);

		// Compile and Create Shaders
		ID3D11VertexShader* CreateVertexShader(const wchar_t* file, const char* entry, char layout);
		ID3D11PixelShader* CreatePixelShader(const wchar_t* file, const char* entry);
	};
}
}