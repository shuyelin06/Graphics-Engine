#include "VisualEngine.h"

#include <assert.h>

#include "math/Vector3.h"
#include "datamodel/shaders/ShaderData.h"

namespace Engine
{
	void VisualEngine::initialize(HWND window)
	{
        /* Initialize Swap Chain */
        // Populate a Swap Chain Structure, responsible for swapping between textures (for rendering)
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

        // Create swap chain, and save pointer data
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
            &swap_chain,
            &device,
            &feature_level,
            &device_context);

        // Check for success
        assert(S_OK == result && swap_chain && device && device_context);


        /* Create Render Target (Output Images) */
        // Obtain Frame Buffer
        ID3D11Texture2D* framebuffer;

        result = swap_chain->GetBuffer(
            0, // Get Buffer 0
            __uuidof(ID3D11Texture2D),
            (void**) &framebuffer);

        // Check for success
        assert(SUCCEEDED(result));

        // Create Render Target with Frame Buffer
        result = device->CreateRenderTargetView(
            framebuffer, 0, &render_target_view);
        // Check Success
        assert(SUCCEEDED(result));

        framebuffer->Release();


        /* Build our Shaders using D3DCompileFromFile() */
        // Compile Vertex Shader
        D3D11_INPUT_ELEMENT_DESC inputElementDesc[] = {
            { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 }
        };
        InputLayoutDescription input_desc = { inputElementDesc, ARRAYSIZE(inputElementDesc) };

        vertex_shader = VertexShader(&input_desc);
        vertex_shader.compileBlob(L"src/shaders/shader.hlsl", "vs_main");
        vertex_shader.createShader(device);

        // Compile Pixel Shader
        pixel_shader = PixelShader();
        pixel_shader.compileBlob(L"src/shaders/shader.hlsl", "ps_main");
        pixel_shader.createShader(device);

        /* Define Vertex Points to Put into our Vertex Shader */
        // This is simply an array of floats - but given our specified input layout,
        // this will be interpreted as an input of float3 types!
        float vertex_data_array[] = {
            0.0f,  0.5f,  0.75f, // point at top
           0.5f, -0.5f,  0.0f, // point at bottom-right
          -0.5f, -0.5f,  0.25f, // point at bottom-left

          0.0f,  0.25f,  0.25f, // point at top
           1.0f, -0.25f, 0.25f, // point at bottom-right
          -0.25f, -0.25f, 0.25f, // point at bottom-left
        };
        UINT vertex_stride = 3 * sizeof(float); // Bytes between the beginning of each vertex
        UINT vertex_offset = 0; // Offset into the buffer to start reading
        UINT vertex_count = 6; // Total number of vertices

        // Load this array into the vertex buffer
        // ID3D11Buffer* vertex_buffer_ptr = NULL; // Create a vertex buffer
        {
            // Descriptor for Vertex buffer
            D3D11_BUFFER_DESC vertex_buff_descr = {};
            vertex_buff_descr.ByteWidth = sizeof(vertex_data_array); // Size of buffer
            vertex_buff_descr.Usage = D3D11_USAGE_DEFAULT; // Usage of buffer (helps with driver optimization)
            vertex_buff_descr.BindFlags = D3D11_BIND_VERTEX_BUFFER; // Vertex Buffer type

            // Subresource Data?
            // Load Data
            D3D11_SUBRESOURCE_DATA sr_data = { 0 };
            sr_data.pSysMem = vertex_data_array;

            // Create Vertex Buffer with data loaded into it
            HRESULT hr = device->CreateBuffer(
                &vertex_buff_descr,
                &sr_data,
                &vertex_buffer_ptr);
            assert(SUCCEEDED(hr));
        }

        /* Attempt Creating Constant Buffer */
        Engine::ShaderData shaderData;
        shaderData.color = Engine::Math::Vector3(0.75f, 0.2f, 0.2f);

        // Fill buffer description
        D3D11_BUFFER_DESC cDesc;
        cDesc.ByteWidth = sizeof(Engine::ShaderData);
        cDesc.Usage = D3D11_USAGE_DYNAMIC;
        cDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        cDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
        cDesc.MiscFlags = 0;
        cDesc.StructureByteStride = 0;

        ID3D11Buffer* g_pConstantBuffer11 = NULL;

        // Fill subresource data
        D3D11_SUBRESOURCE_DATA InitData;
        InitData.pSysMem = &shaderData;
        InitData.SysMemPitch = 0;
        InitData.SysMemSlicePitch = 0;

        result = device->CreateBuffer(
            &cDesc, &InitData, &g_pConstantBuffer11
        );

        assert(SUCCEEDED(result));
            
        device_context->PSSetConstantBuffers(0, 1, &g_pConstantBuffer11);
	}

    // Render a Frame
    void VisualEngine::render(HWND window)
    {
        /* Handle Rendering with Direct3D */
        // Clear back buffer (and depth buffer if available)
        float background_color[4] = {
             0x64 / 255.0f, 0x95 / 255.0f, 0xED / 255.0f, 1.0f };
        device_context->ClearRenderTargetView(
            render_target_view, background_color);

        // Set the valid drawing area (our window)
        RECT winRect;
        GetClientRect(window, &winRect); // Get the windows rectangle

        D3D11_VIEWPORT viewport = {
            0.0f, 0.0f,
            (FLOAT)(winRect.right - winRect.left),
            (FLOAT)(winRect.bottom - winRect.top),
            0.0f,
            1.0f
        };
        // Give rectangle to rasterizer state function
        device_context->RSSetViewports(1, &viewport);

        // Set output merger to use our render target
        device_context->OMSetRenderTargets(1, &render_target_view, NULL);

        // Set the Input Assembler to use our vertex buffer
        // Triangle list - every 3 vertices forms a separate triangle
        UINT vertex_stride = 3 * sizeof(float); // Bytes between the beginning of each vertex
        UINT vertex_offset = 0; // Offset into the buffer to start reading
        UINT vertex_count = 6; // Total number of vertices

        device_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST); // Set primitive format 
        device_context->IASetInputLayout(vertex_shader.getInputLayout()); // Set input layout
        device_context->IASetVertexBuffers( // Use the vertex buffer we created!
            0,
            1,
            &vertex_buffer_ptr,
            &vertex_stride,
            &vertex_offset
        );

        // Set the Vertex Shader
        device_context->VSSetShader(
            static_cast<ID3D11VertexShader*>(vertex_shader.getShader()), NULL, 0);

        // Set the pixel shader
        device_context->PSSetShader(
            static_cast<ID3D11PixelShader*>(pixel_shader.getShader()), NULL, 0);

        // Draw 3 vertices from our vertex buffer
        // Draw will use all of the states we set, the vertex buffer and shaders.
        // We specify how many vertices to draw from our vertex buffer.
        device_context->Draw(vertex_count, 0);

        // Now that this has been rendered, we use the swap chain and swap the buffers, to show the screen
        swap_chain->Present(1, 0);
    }

	// Create device interface and swapchain to swap between textures
	void VisualEngine::create_device_swapchain(HWND window)
	{
		// Configure swap chain descriptor
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

		// Use descriptor to create swap chain
		// Create swap chain, and save pointer data
		D3D_FEATURE_LEVEL feature_level;
		UINT flags = D3D11_CREATE_DEVICE_SINGLETHREADED;
#if defined( DEBUG ) || defined( _DEBUG )
		flags |= D3D11_CREATE_DEVICE_DEBUG;
#endif
		// Create Device and Swap Chain using our created struct
		HRESULT hr = D3D11CreateDeviceAndSwapChain(
			NULL,
			D3D_DRIVER_TYPE_HARDWARE,
			NULL,
			flags,
			NULL,
			0,
			D3D11_SDK_VERSION,
			&swap_chain_descriptor,
			&swap_chain,
			&device,
			&feature_level,
			&device_context);
		
		// Check for success
		assert(S_OK == hr && swap_chain && device && device_context);
	}

	// Create the Render Targets (Output Images)
	void VisualEngine::create_render_target()
	{
		// Obtain Frame Buffer
		ID3D11Texture2D* framebuffer;
		HRESULT hr = swap_chain->GetBuffer(
			0,
			__uuidof(ID3D11Texture2D),
			(void**)&framebuffer);
		// Check if we've successfully obtained the frame buffer 
		assert(SUCCEEDED(hr));

		// Create Render Target with Frame Buffer
		hr = device->CreateRenderTargetView(
			framebuffer, 0, &render_target_view);
		// Check Success
		assert(SUCCEEDED(hr));

		framebuffer->Release();
	}

	void VisualEngine::compile_shaders()
	{
        // Configure shader compilation flags
        UINT flags = D3DCOMPILE_ENABLE_STRICTNESS; // Set flags
#if defined(DEBUG) || defined(_DEBUG)
        flags |= D3DCOMPILE_DEBUG; // More Debug Output
#endif

        HRESULT hr; // Stores function results
        ID3DBlob* error_blob = NULL; // Stores (potential) errors

        /* Compile Vertex Shader */
        ID3DBlob* vs_blob_ptr = NULL;

        hr = D3DCompileFromFile( // Compile blob
            L"src/shaders/shader.hlsl",
            nullptr,
            D3D_COMPILE_STANDARD_FILE_INCLUDE,
            "vs_main", // Entry Point
            "vs_5_0", // Vertex Shader Compiler Target
            flags, // Flags
            0,
            &vs_blob_ptr,
            &error_blob);

        if (FAILED(hr)) { // Verify success
            // Check Error Blob
            if (error_blob) {
                // Output Debug String
                OutputDebugStringA((char*)error_blob->GetBufferPointer());
                // Release (Free) Error Blob
                error_blob->Release();
            }
            // Release the Vertex Shader Blob if Allocated
            if (vs_blob_ptr) { vs_blob_ptr->Release(); }
            assert(false);
        }
        
        ID3D11VertexShader* vertex_shader_ptr = NULL;
        hr = device->CreateVertexShader( // Create Vertex Shader
            vs_blob_ptr->GetBufferPointer(),
            vs_blob_ptr->GetBufferSize(),
            NULL,
            &vertex_shader_ptr);
        assert(SUCCEEDED(hr));

        
        /* Compile Pixel Shader */
        ID3DBlob* ps_blob_ptr = NULL;

        hr = D3DCompileFromFile( // Compile blob
            L"src/shaders/shader.hlsl",
            nullptr,
            D3D_COMPILE_STANDARD_FILE_INCLUDE,
            "ps_main", // Entry Point
            "ps_5_0", // PixelShader Compiler Target
            flags,
            0,
            &ps_blob_ptr,
            &error_blob);
        
        // Check for Success
        if (FAILED(hr)) {
            // Check Error Blob
            if (error_blob) {
                // Output Debug String
                OutputDebugStringA((char*)error_blob->GetBufferPointer());
                // Release Error Blob
                error_blob->Release();
            }
            // Release the Pixel Shader Blob if Allocated
            if (ps_blob_ptr) { ps_blob_ptr->Release(); }
            assert(false);
        }

        ID3D11PixelShader* pixel_shader_ptr = NULL;
        
        hr = device->CreatePixelShader( // Create Pixel Shader
            ps_blob_ptr->GetBufferPointer(),
            ps_blob_ptr->GetBufferSize(),
            NULL,
            &pixel_shader_ptr);
        assert(SUCCEEDED(hr));
	}

    void VisualEngine::create_buffers()
    {

    }
}