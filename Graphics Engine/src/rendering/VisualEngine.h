#pragma once

// Direct 3D 11 Library Includes
#include <d3d11.h> // Direct 3D Interface
#include <dxgi.h> // DirectX Driver Interface
#include <d3dcompiler.h> // Shader Compiler

// Indicates Visual C++ to leave a command in the object file, which can be read by
// the linker when it processes object files.
// Tells the linker to add the "library" library to the list of library dependencies
#pragma comment( lib, "user32" )          // link against the win32 library
#pragma comment( lib, "d3d11.lib" )       // direct3D library
#pragma comment( lib, "dxgi.lib" )        // directx graphics interface
#pragma comment( lib, "d3dcompiler.lib" ) // shader compiler

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
