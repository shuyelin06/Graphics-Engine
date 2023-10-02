#pragma once

#include "Direct3D11.h"

#include "Shaders.h"

namespace Engine
{
	class VisualEngine
	{
	private:
		// Direct 3D 11 Pointers
		ID3D11Device* device;
		ID3D11DeviceContext* device_context;
		IDXGISwapChain* swap_chain;
		ID3D11RenderTargetView* render_target_view;

		VertexShader vertex_shader;
		PixelShader pixel_shader;

		ID3D11Buffer* vertex_buffer_ptr;

	public:
		void initialize(HWND window); // Initialize Direct 3D
		void render(HWND window); // Render a Frame

	private:
		void create_device_swapchain(HWND window);
		void create_render_target();
		void compile_shaders();
		void create_buffers();
	};
}
