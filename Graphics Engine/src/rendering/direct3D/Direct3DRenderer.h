#pragma once

#include "Direct3D11.h"
#include "Direct3DBuffer.h"

#include "rendering/core/Renderer.h"

namespace Engine
{

namespace Graphics
{
	
	// Direct3DRenderer
	// Represents a Direct3DRenderer, which will
	// interface with the Direct3D graphics API
	class Direct3DRenderer : public Renderer
	{
	private:
		HWND window;

		ID3D11Device* device;
		ID3D11DeviceContext* device_context;
		
		IDXGISwapChain* swapchain;
		ID3D11RenderTargetView* render_target;

	public:
		Direct3DRenderer(HWND);
		~Direct3DRenderer();

		void Render() override;

		void BindVertexBuffer(InputTopology, Buffer*, unsigned int vertex_size) override;
		void BindConstantBuffer(BufferTarget, Buffer*, int index) override;
		
		void BindVertexShader(VertexShader*) override;
		void BindPixelShader(PixelShader*) override;

		Buffer* CreateBuffer(void *data, int size) override;

		PixelShader* CreatePixelShader(const wchar_t* file, const char* entry) override;
		VertexShader* CreateVertexShader(const wchar_t* file, const char* entry, std::vector<InputLayout>&) override;
	};
}
}