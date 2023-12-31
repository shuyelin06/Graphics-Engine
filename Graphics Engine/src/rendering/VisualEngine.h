#pragma once

#include "Direct3D11.h"

#include "buffers/VertexBuffer.h"
#include "objects/other/Camera.h"
#include "objects/Object.h"

#include <vector>
#include <utility>

namespace Engine
{

namespace Graphics
{
	typedef enum { Vertex, Pixel } Shader_Type;

	class VisualEngine
	{
	private:
		// Window Handle
		HWND window;

		// Direct 3D 11 Pointers
		ID3D11Device* device;
		ID3D11DeviceContext* device_context;
		IDXGISwapChain* swap_chain;
		ID3D11RenderTargetView* render_target_view;

		// Available Constant Buffers
		std::vector<ID3D11Buffer*> vs_constant_buffers;
		std::vector<ID3D11Buffer*> ps_constant_buffers;

		// Available Shaders
		std::vector<std::pair<ID3D11VertexShader*, ID3D11InputLayout*>> vertex_shaders; // Vertex Shader and Associated Input Layout
		std::vector<ID3D11PixelShader*> pixel_shaders; // Pixel Shaders

		// Currently Selected Vertex and Pixel Shaders
		std::pair<ID3D11VertexShader*, ID3D11InputLayout*> cur_vertex_shader;
		ID3D11PixelShader* cur_pixel_shader;

	public:
		VisualEngine();
		void initialize(HWND _window); // Initialize Direct 3D
		
		// Rendering Methods
		void clear_screen(float color[4]); // Clear Screen

		// Generate a renderable vertex buffer
		VertexBuffer generate_vertex_buffer(void *vertices, int floats_per_vertex, int num_vertices);

		// Bind data to the vertex and pixel shaders
		void bind_vs_data(unsigned int index, void* data, int byte_size);
		void bind_ps_data(unsigned int index, void* data, int byte_size);

		// Bind shaders to the rendering pipeline
		void bind_vertex_shader(int index);
		void bind_pixel_shader(int index);
		
		// Draws an object from the camera's point of view
		void drawObject(Datamodel::Camera* camera, Datamodel::Object* object);

		void present(); // Present Drawn Content to Screen

	private:
		// Create Buffers
		ID3D11Buffer* create_buffer(D3D11_BIND_FLAG, void *data, int byte_size);

		// Helper Bind Data Method
		void bind_data(Shader_Type type, unsigned int index, void* data, int byte_size);

		// Compile and Create Shaders
		void create_vertex_shader(const wchar_t* file, const char* entry, D3D11_INPUT_ELEMENT_DESC[], int desc_size);
		void create_pixel_shader(const wchar_t* file, const char* entry);
	};
}
}