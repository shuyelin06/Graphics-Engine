#include "Direct3DRenderer.h"

#include "Direct3DVertexShader.h"
#include "Direct3DPixelShader.h"
#include "Direct3DBuffer.h"

namespace Engine
{

namespace Graphics
{
	// --- Constructors ---
	// Constructor: Creates a Direct3DDevice, to be used
	// to interface with the Direct3D Graphics API
	Direct3DRenderer::Direct3DRenderer(HWND _window)
	{
        window = _window;
        // Preinitialize all member variables to nullptr
        device = nullptr;
        device_context = nullptr;
        
        swapchain = nullptr;
        render_target = nullptr; 

		// Initialize Device, Device Context, and Swapchain
        DXGI_SWAP_CHAIN_DESC swap_chain_descriptor = { 0 };
        swap_chain_descriptor.BufferDesc.RefreshRate.Numerator = 0; // Synchronize Output Frame Rate
        swap_chain_descriptor.BufferDesc.RefreshRate.Denominator = 1;
        swap_chain_descriptor.BufferDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM; // Color Output Format
        swap_chain_descriptor.SampleDesc.Count = 1;
        swap_chain_descriptor.SampleDesc.Quality = 0;
        swap_chain_descriptor.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        swap_chain_descriptor.BufferCount = 1; // Number of back buffers to add to the swap chain
        swap_chain_descriptor.OutputWindow = window;
        swap_chain_descriptor.Windowed = true; // Displaying to a Window

        // Specifies feature level, a defined set of GPU functionalities
        // (for compatibility with other Direct3D versions)
        D3D_FEATURE_LEVEL feature_level;

        HRESULT result = D3D11CreateDeviceAndSwapChain(
            NULL,
            D3D_DRIVER_TYPE_HARDWARE,
            NULL,
            D3D11_CREATE_DEVICE_SINGLETHREADED, // Flags
            NULL,
            0,
            D3D11_SDK_VERSION,
            &swap_chain_descriptor,
            &swapchain,
            &device,
            &feature_level,
            &device_context);
        
        // Obtain the 0th frame buffer from swapchain,
        // to create a render target with
        ID3D11Texture2D* framebuffer;

        swapchain->GetBuffer(
            0,
            __uuidof(ID3D11Texture2D),
            (void**) &framebuffer
        );

        // Create a render target with this frame buffer
        device->CreateRenderTargetView(
            framebuffer, 0, &render_target
        );

        // Release frame buffer
        framebuffer->Release();
	}

    // Destructor
    // Ensures all allocated memory is deallocated
    Direct3DRenderer::~Direct3DRenderer() {}

    // --- Operations ---
    // Render
    // Renders to the screen
    void Direct3DRenderer::Render()
    {
        // Clear back buffer (and depth buffer if available)
        float background_color[4] = {
             0x64 / 255.0f, 0x95 / 255.0f, 0xED / 255.0f, 1.0f };
        device_context->ClearRenderTargetView(
            render_target, background_color);

        // Set the valid drawing area
        RECT winRect;
        GetClientRect(window, &winRect); // Get the windows rectangle

        D3D11_VIEWPORT viewport = {
            0.0f, 0.0f,
            (FLOAT)(winRect.right - winRect.left),
            (FLOAT)(winRect.bottom - winRect.top),
            0.0f,
            1.0f
        };

        // Set the rasterizer
        device_context->RSSetViewports(1, &viewport);

        // Set the output merger to use our render target 
        device_context->OMSetRenderTargets(1, &render_target, NULL);
        
        // Draw and present
        device_context->Draw(6, 0);

        swapchain->Present(1, 0);
    }

    // Bind resources to the renderer
    void Direct3DRenderer::BindVertexBuffer(InputTopology input_topology, Buffer* buffer, unsigned int vertex_size)
    {
        // Set topology
        D3D_PRIMITIVE_TOPOLOGY topology = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
        device_context->IASetPrimitiveTopology(topology);

        // Set vertex buffer
        UINT vertex_stride = vertex_size;
        UINT vertex_offset = 0;

        ID3D11Buffer** buffer_addr = static_cast<Direct3DBuffer*>(buffer)->getBufferAddress();
        device_context->IASetVertexBuffers(0, 1, buffer_addr, &vertex_stride, &vertex_offset);
    }

    void Direct3DRenderer::BindConstantBuffer(BufferTarget target, Buffer* buffer, int index)
    {
        ID3D11Buffer** buffer_addr = static_cast<Direct3DBuffer*>(buffer)->getBufferAddress();

        if (target == Vertex)
        {
            device_context->VSSetConstantBuffers(index, 1, buffer_addr);
        }
        else if (target == Pixel)
        {
            device_context->PSSetConstantBuffers(index, 1, buffer_addr);
        }
    }

    void Direct3DRenderer::BindVertexShader(VertexShader* shader)
    {
        Direct3DVertexShader* v_shader = static_cast<Direct3DVertexShader*>(shader);
        device_context->IASetInputLayout(v_shader->getInputLayout());
        device_context->VSSetShader(v_shader->getShader(), NULL, 0);
    }

    void Direct3DRenderer::BindPixelShader(PixelShader* shader)
    {
        Direct3DPixelShader* p_shader = static_cast<Direct3DPixelShader*>(shader);
        device_context->PSSetShader(p_shader->getShader(), NULL, 0);
    }

    // Create a buffer for use
    Buffer* Direct3DRenderer::CreateBuffer(void* data, int size)
    {
        return new Direct3DBuffer(device, data, size);
    }

    // TODO
    PixelShader* Direct3DRenderer::CreatePixelShader(const wchar_t* shader_file, const char* entrypoint)
    {
        return new Direct3DPixelShader(device, shader_file, entrypoint);
    }

    VertexShader* Direct3DRenderer::CreateVertexShader(const wchar_t* shader_file, const char* entrypoint, std::vector<InputLayout>& layout)
    {
        return new Direct3DVertexShader(device, shader_file, entrypoint, layout);
    }

}
}