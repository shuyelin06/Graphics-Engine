#pragma once

#include "Direct3D11.h"

#include "buffers/VertexBuffer.h"

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
		// Handle to Window
		HWND window;

		// Direct 3D 11 Pointers
		ID3D11Device* device;
		ID3D11DeviceContext* device_context;
		IDXGISwapChain* swap_chain;
		ID3D11RenderTargetView* render_target_view;

		// Vectors of Available Shaders
		std::vector<std::pair<ID3D11VertexShader*, ID3D11InputLayout*>> vertex_shaders; // Vertex Shader and Associated Input Layout
		std::vector<ID3D11PixelShader*> pixel_shaders; // Pixel Shaders

		// Currently Selected Vertex and Pixel Shaders
		std::pair<ID3D11VertexShader*, ID3D11InputLayout*> cur_vertex_shader;
		ID3D11PixelShader* cur_pixel_shader;

	public:
		VisualEngine(HWND _window);
		void initialize(); // Initialize Direct 3D
		
		// Rendering Methods
		void clear_screen(float color[4]); // Clear Screen

		void bind_data(Shader_Type shader, int index, void* data, int byte_size);
		void bind_vertex_shader(int index);
		void bind_pixel_shader(int index);
		
		void draw(VertexBuffer buffer); // Draw a Vertex List

		void present(); // Present Drawn Content to Screen

	private:
		// Create Buffers
		ID3D11Buffer* create_buffer(D3D11_BIND_FLAG, void *data, int byte_size);

		// Compile and Create Shaders
		void create_vertex_shader(const wchar_t* file, const char* entry, D3D11_INPUT_ELEMENT_DESC[], int desc_size);
		void create_pixel_shader(const wchar_t* file, const char* entry);
	};
}
}