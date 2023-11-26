#include "VisualEngine.h"

#include <assert.h>

#include "math/Vector3.h"
#include "datamodel/shaders/ShaderData.h"

namespace Engine
{
    /* --- Constructor --- */
    // Saves the handle to the application window
    VisualEngine::VisualEngine(HWND _window)
    {
        window = _window;

        device = NULL;
        device_context = NULL;
        swap_chain = NULL;
        render_target_view = NULL;
    }

    /* --- Initialize --- */
    // Initializes Direct3D11 
    void VisualEngine::initialize()
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
            (void**)&framebuffer);

        // Check for success
        assert(SUCCEEDED(result));

        // Create Render Target with Frame Buffer
        result = device->CreateRenderTargetView(
            framebuffer, 0, &render_target_view);
        // Check Success
        assert(SUCCEEDED(result));

        framebuffer->Release();


        /* Build our Shaders */
        // Compile Vertex Shader
        D3D11_INPUT_ELEMENT_DESC input_desc[] = {
            { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 }
        };
        create_vertex_shader(L"src/shaders/shader.hlsl", "vs_main", input_desc, ARRAYSIZE(input_desc));

        // Create Pixel Shader
        create_pixel_shader(L"src/shaders/shader.hlsl", "ps_main");
    }

    /* --- Rendering --- */
    // The below functions can be called for rendering

    // Clears the screen with a color
    void VisualEngine::clear_screen(float color[4])
    {
        device_context->ClearRenderTargetView(render_target_view, color);
    }
    
    // Binds a vertex shader to be used in the rendering
    // pipeline
    void VisualEngine::bind_vertex_shader(int index)
    {
        if (0 <= index && index < vertex_shaders.size())
        {
            cur_vertex_shader = vertex_shaders[index];
        }
        else assert(false);
    }
    
    // Binds a pixel shader to be used in the rendering
    // pipeline
    void VisualEngine::bind_pixel_shader(int index)
    {
        if (0 <= index && index < pixel_shaders.size())
        {
            cur_pixel_shader = pixel_shaders[index];
        }
        else assert(false);
    }

    // Run a draw call, passing a list of vertices through the
    // graphics pipeline
    void VisualEngine::draw(VertexBuffer buffer)
    {
        // Metadata for Vertex Buffer
        UINT vertex_stride = buffer.vertex_size * sizeof(float); // Bytes between the beginning of each vertex
        UINT vertex_offset = 0; // Offset into the buffer to start reading
        UINT vertex_count = buffer.num_vertices; // Total number of vertices

        // Create Vertex Buffer
        ID3D11Buffer* vertex_buffer_ptr;

        {
            // Descriptor for Vertex buffer
            D3D11_BUFFER_DESC vertex_buff_descr = {};
            vertex_buff_descr.ByteWidth = vertex_stride * vertex_count; // Size of buffer
            vertex_buff_descr.Usage = D3D11_USAGE_DEFAULT; // Usage of buffer (helps with driver optimization)
            vertex_buff_descr.BindFlags = D3D11_BIND_VERTEX_BUFFER; // Vertex Buffer type

            // Load Data
            D3D11_SUBRESOURCE_DATA sr_data = { 0 };
            sr_data.pSysMem = buffer.vertices;

            // Create Vertex Buffer with data loaded into it
            HRESULT hr = device->CreateBuffer(
                &vertex_buff_descr,
                &sr_data,
                &vertex_buffer_ptr);

            // Handle Faillure
            assert(SUCCEEDED(hr));
        }

        // Perform a Draw Call
        // Set the valid drawing area (our window)
        RECT winRect;
        GetClientRect(window, &winRect); // Get the windows rectangle

        D3D11_VIEWPORT viewport = {
            0.0f, 0.0f,
            (FLOAT) (winRect.right - winRect.left),
            (FLOAT) (winRect.bottom - winRect.top),
            0.0f,
            1.0f
        };

        // Give rectangle to rasterizer state function
        device_context->RSSetViewports(1, &viewport);

        // Set output merger to use our render target
        device_context->OMSetRenderTargets(1, &render_target_view, NULL);

        // Set input
        device_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        device_context->IASetInputLayout(cur_vertex_shader.second);

        device_context->IASetVertexBuffers(
            0,
            1,
            &vertex_buffer_ptr,
            &vertex_stride,
            &vertex_offset
        );

        // Bind shaders
        device_context->VSSetShader(cur_vertex_shader.first, NULL, 0);
        device_context->PSSetShader(cur_pixel_shader, NULL, 0);

        // Draw 3 vertices from our vertex buffer
        // Draw will use all of the states we set, the vertex buffer and shaders.
        // We specify how many vertices to draw from our vertex buffer.
        device_context->Draw(vertex_count, 0);
    }

    // Swaps the swapchain buffers, presenting drawn content
    // to the screen
    void VisualEngine::present()
    {
        swap_chain->Present(1, 0);
    }

    /* --- Shader Creation --- */
    // The below functions can be used to compile shaders
    // for the graphics engine.
    
    // Compiles a shader blob
    typedef enum {Vertex, Pixel} Shader_Type;
    static ID3DBlob* compile_shader_blob(Shader_Type type, const wchar_t* file, const char* entry)
    {
        // Initialize compiler settings
        ID3DInclude* include_settings = D3D_COMPILE_STANDARD_FILE_INCLUDE;
        const char* compiler_target = "";
        const UINT flags = 0 | D3DCOMPILE_ENABLE_STRICTNESS;

        switch (type) {
        case Vertex:
            compiler_target = "vs_5_0";
            break;

        case Pixel:
            compiler_target = "ps_5_0";
            break;
        }

        // Compile blob
        ID3DBlob* error_blob = NULL;
        ID3DBlob* compiled_blob = NULL;

        HRESULT result = D3DCompileFromFile(
            file, 
            nullptr,
            include_settings,
            entry,
            compiler_target,
            flags,
            0,
            &compiled_blob,
            &error_blob
        );

        // Error handling
        if (FAILED(result))
        {
            // Print error if message exists
            if (error_blob)
            {
                OutputDebugStringA((char*)error_blob->GetBufferPointer());
                error_blob->Release();
            }
            // Release shader blob if allocated
            if (compiled_blob) { compiled_blob->Release(); }
            assert(false);
        }

        return compiled_blob;
    }

    // Creates a vertex shader and adds it to the array of vertex shaders
    void VisualEngine::create_vertex_shader(const wchar_t* filename, const char* entrypoint, D3D11_INPUT_ELEMENT_DESC layout_desc[], int desc_size)
    {
        // Obtain shader blob
        ID3DBlob* shader_blob = compile_shader_blob(Vertex, filename, entrypoint);

        // Create input layout for vertex shader
        ID3D11InputLayout* input_layout = NULL;

        device->CreateInputLayout(
            layout_desc,
            desc_size,
            shader_blob->GetBufferPointer(),
            shader_blob->GetBufferSize(),
            &input_layout
        );

        // Check for success
        assert(input_layout != NULL);

        // Create vertex shader
        ID3D11VertexShader* vertex_shader = NULL;

        device->CreateVertexShader(
            shader_blob->GetBufferPointer(),
            shader_blob->GetBufferSize(),
            NULL,
            &vertex_shader
        );

        // Add to array of vertex shaders
        std::pair<ID3D11VertexShader*, ID3D11InputLayout*> pair;
        pair.first = vertex_shader;
        pair.second = input_layout;

        vertex_shaders.push_back(pair);
    }

    // Creates a pixel shader and adds it to the array of pixel shaders
    void VisualEngine::create_pixel_shader(const wchar_t* filename, const char* entrypoint)
    {
        // Obtain shader blob
        ID3DBlob* shader_blob = compile_shader_blob(Pixel, filename, entrypoint);

        // Create pixel shader
        ID3D11PixelShader* pixel_shader = NULL;

        device->CreatePixelShader(
            shader_blob->GetBufferPointer(),
            shader_blob->GetBufferSize(),
            NULL,
            &pixel_shader
        );

        // Check for success
        assert(pixel_shader != NULL);

        // Add to array of pixel shaders
        pixel_shaders.push_back(pixel_shader);
    }

}