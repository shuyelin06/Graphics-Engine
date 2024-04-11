#include "VisualAttribute.h"

#include <assert.h>
#include <d3d11_1.h>

#include "datamodel/Scene.h"
#include "datamodel/Object.h"
#include "datamodel/Camera.h"

namespace Engine
{

namespace Graphics
{
    // VertexLayoutSize:
    // Static method returning the number of floats a given 
    // vertex layout has
    int VertexLayoutSize(char layout)
    {
        int size = ((layout & XYZ) * 3)	// 1st Bit: XYZ Position
            + (((layout & RGB) >> 1) * 3)	// 2nd Bit: RGB Color
            + (((layout & NORMAL) >> 2) * 3); // 3rd Bit: XYZ Normal
        return size;
    }

    // LayoutHasPin:
    // Checks if a layout has a given pin or not
    bool LayoutHasPin(char layout, VertexLayoutPin pin)
    {
        return (layout & pin) == pin;
    }

    /* Static Class Definitions and Declarations */
    // Win32 Window Handle
    HWND VisualAttribute::window = nullptr;
    
    // Direct3D Pointers
    ID3D11Device* VisualAttribute::device = NULL;
    ID3D11DeviceContext* VisualAttribute::device_context = NULL;
    IDXGISwapChain* VisualAttribute::swap_chain = NULL;

    ID3D11RenderTargetView* VisualAttribute::render_target_view = NULL;
    ID3D11DepthStencilView* VisualAttribute::depth_stencil = NULL;

    // Available Constant Buffers
    vector<ID3D11Buffer*> VisualAttribute::vs_buffers = vector<ID3D11Buffer*>();
    vector<ID3D11Buffer*> VisualAttribute::ps_buffers = vector<ID3D11Buffer*>();

    // Pipeline Assignable Elements
    map<char, ID3D11InputLayout*> VisualAttribute::input_layouts = map<char, ID3D11InputLayout*>();
    vector<ID3D11VertexShader*> VisualAttribute::vertex_shaders = vector<ID3D11VertexShader*>();
    vector<ID3D11PixelShader*> VisualAttribute::pixel_shaders = vector<ID3D11PixelShader*>();

    // Visual Attributes to Render
    vector<VisualAttribute*> VisualAttribute::attributes = vector<VisualAttribute*>();

    Camera* VisualAttribute::camera = nullptr;

    // Initialize:
    // Initializes Direct3D and other Visual Attributes
    void VisualAttribute::Initialize(HWND _window)
    {
        // Set Window Handle
        window = _window;

        // Get window width and height
        RECT rect;
        GetWindowRect(window, &rect);

        int width = rect.right - rect.left;
        int height = rect.bottom - rect.top;

        /* Initialize Swap Chain */
        // Populate a Swap Chain Structure, responsible for swapping between textures (for rendering)
        DXGI_SWAP_CHAIN_DESC swap_chain_descriptor = { 0 };
        swap_chain_descriptor.BufferDesc.RefreshRate.Numerator = 0; // Synchronize Output Frame Rate
        swap_chain_descriptor.BufferDesc.RefreshRate.Denominator = 1;
        swap_chain_descriptor.BufferDesc.Width = width;
        swap_chain_descriptor.BufferDesc.Height = height;
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

        // Create Render Target with Frame Buffer
        {
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
        }

        // Release frame buffer (no longer needed)
        framebuffer->Release();

        // Create 2D texture to be used as a depth stencil
        ID3D11Texture2D* depth_texture = NULL;
        {
            // Create description for the texture
            D3D11_TEXTURE2D_DESC desc_texture= {};
            desc_texture.Width = width;
            desc_texture.Height = height;
            desc_texture.MipLevels = 1;
            desc_texture.ArraySize = 1;
            desc_texture.MipLevels = 1;
            desc_texture.ArraySize = 1;
            desc_texture.SampleDesc.Count = 1;
            desc_texture.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
            desc_texture.BindFlags = D3D11_BIND_DEPTH_STENCIL;
            
            // Create 2D texture for depth stencil
            result = device->CreateTexture2D(&desc_texture, NULL, &depth_texture);

            // Check for failure
            if (result != S_OK)
                assert(false);
        }

        // Create a depth stencil from the 2D texture
        {
            // Create description for the depth stencil
            D3D11_DEPTH_STENCIL_VIEW_DESC desc_stencil = {};
            desc_stencil.Format = DXGI_FORMAT_D24_UNORM_S8_UINT; // Same format as texture
            desc_stencil.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DMS;

            // Create depth stencil
            device->CreateDepthStencilView(depth_texture, &desc_stencil, &depth_stencil);
        }

        /* Build our Shaders */
        // Compile Vertex Shader
        CreateVertexShader(L"src/shaders/shader.hlsl", "vs_main", XYZ | NORMAL);

        // Create Pixel Shader
        CreatePixelShader(L"src/shaders/shader.hlsl", "ps_main");

        InitializeLineHandler();
    }

    // RenderAll:
    // Renders all VisualAttributes subscribed for rendering
    void VisualAttribute::RenderAll()
    {
        // Query lines to draw
        DrawLine({ -10,0,0 }, {10,0,0}, {1,0,0});
        DrawLine({ 0,-10,0 }, { 0,10,0 }, {0,1,0});
        DrawLine({ 0,0,-10 }, { 0,0,10 }, {0,0,1});

        // Clear screen
        float color[4] = { 0, 0, 0, 1.0f };
        device_context->ClearRenderTargetView(render_target_view, color);
        
        // Clear depth stencil
        device_context->ClearDepthStencilView(depth_stencil, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0.0f);

        // Prepare line buffer
        PrepareLines();

        // Prepare all visual attributes
        for (VisualAttribute* attr : attributes)
            attr->prepare();

        // Render all visual attributes
        for (VisualAttribute* attr : attributes)
            attr->render();

        // Draw line buffer
        RenderLines(); 

        // Finish all visual attributes
        for (VisualAttribute* attr : attributes)
            attr->finish();
        
        // Present to screen
        swap_chain->Present(1, 0);
    }

    // SetCamera:
    // Sets the camera (TEMP)
    void VisualAttribute::SetCamera(Camera* _camera)
    {
        camera = _camera;
    }

    // ObjectAccessor:
    // Get an object accessor
    ObjectAccessor VisualAttribute::GetObjectAccessor(void)
    {
        return ObjectAccessor();
    }

    // CompileShaderBlob:
    // Compiles a shader blob from a given file
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

    // CreateVertexShader:
    // Creates a vertex shader and adds it to the array of vertex shaders to be used
    int VisualAttribute::CreateVertexShader(const wchar_t* file, const char* entry, char layout)
    {
        // Obtain shader blob
        ID3DBlob* shader_blob = compile_shader_blob(Vertex, file, entry);
        
        // Create vertex shader
        ID3D11VertexShader* vertex_shader = NULL;

        device->CreateVertexShader(
            shader_blob->GetBufferPointer(),
            shader_blob->GetBufferSize(),
            NULL,
            &vertex_shader
        );

        // Add to array of vertex shaders
        int index = vertex_shaders.size();
        vertex_shaders.push_back(vertex_shader);

        // Generate input layout if it does not already exist
        if (!input_layouts.contains(layout))
        {
            vector<D3D11_INPUT_ELEMENT_DESC> input_desc = vector<D3D11_INPUT_ELEMENT_DESC>();

            // XYZ Layout Pin
            if (LayoutHasPin(layout, XYZ))
                input_desc.push_back({ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 });
            // Color Layout Pin
            if (LayoutHasPin(layout, RGB))
                input_desc.push_back({ "COLOR", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 });
            // Normal Layout Pin
            if (LayoutHasPin(layout, NORMAL))
                input_desc.push_back({ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 });

            // Generate input layout
            ID3D11InputLayout* input_layout = NULL;

            device->CreateInputLayout(
                input_desc.data(),
                input_desc.size(),
                shader_blob->GetBufferPointer(),
                shader_blob->GetBufferSize(),
                &input_layout
            );

            // Add to input layouts
            input_layouts[layout] = input_layout;
        }

        return index;
    }

    // CreatePixelShader:
    // Creates a pixel shader and adds it to the array of pixel shaders
    // for use
    int VisualAttribute::CreatePixelShader(const wchar_t* filename, const char* entrypoint)
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
        int size = pixel_shaders.size();
        pixel_shaders.push_back(pixel_shader);

        return size;
    }

    // CreateBuffer:
    // Creates a generic buffer that can be used throughout our graphics pipeline. 
    // Uses include but are not limited to:
    // 1) Constant Buffers (without resource renaming)
    // 2) Vertex Buffers
    // 3) Index Buffers 
    ID3D11Buffer* VisualAttribute::CreateBuffer(D3D11_BIND_FLAG bind_flag, void* data, int byte_size)
    {
        ID3D11Buffer* buffer = NULL;

        // Fill buffer description
        D3D11_BUFFER_DESC buff_desc = {};

        buff_desc.ByteWidth = byte_size;
        buff_desc.Usage = D3D11_USAGE_DEFAULT;
        buff_desc.BindFlags = bind_flag;

        // Fill subresource data
        D3D11_SUBRESOURCE_DATA sr_data = { 0 };
        sr_data.pSysMem = data;

        // Create buffer
        HRESULT result = device->CreateBuffer(
            &buff_desc, &sr_data, &buffer
        );

        assert(SUCCEEDED(result));

        return buffer;
    }

    // BindData:
    // Uses dynamic resource renaming to fairly efficiently bind data to the vertex / pixel
    // shader constant registers, for use in the shaders
    void VisualAttribute::BindVSData(unsigned int index, void* data, int byte_size)
        { bind_data(Vertex, index, data, byte_size); }

    void VisualAttribute::BindPSData(unsigned int index, void* data, int byte_size)
        { bind_data(Pixel, index, data, byte_size); }

    void VisualAttribute::bind_data(Shader_Type type, unsigned int index, void* data, int byte_size)
    {
        // Get vertex or pixel buffers depending on what's requested
        std::vector<ID3D11Buffer*> buffers = (type == Vertex) ? vs_buffers : ps_buffers;

        // Check if our buffer index exists, and if it doesn't,
        // resize our vector so that it's included
        if (index >= buffers.size())
            buffers.resize(index + 1);

        // Create buffer if it does not exist at that index
        if (buffers[index] == NULL)
        {
            // Create buffer description 
            D3D11_BUFFER_DESC buff_desc = {};

            buff_desc.ByteWidth = byte_size;                    // # Bytes Input Data Takes
            buff_desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;   // Constant Buffer
            buff_desc.Usage = D3D11_USAGE_DYNAMIC;              // Accessible by GPU Read + CPU Write
            buff_desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;  // Allow CPU Writes

            // Allocate resources
            D3D11_SUBRESOURCE_DATA sr_data;

            sr_data.pSysMem = data;
            sr_data.SysMemPitch = 0;
            sr_data.SysMemSlicePitch = 0;

            // Create buffer
            device->CreateBuffer(&buff_desc, &sr_data, &buffers[index]);
        }
        // If buffer exists, perform resource renaming to update buffer data 
        // instead of creating a new buffer
        else
        {
            // Will store data for the already existing shader buffer 
            D3D11_MAPPED_SUBRESOURCE mapped_resource = { 0 };

            // Disable GPU access to data and obtain the resource
            device_context->Map(buffers[index], 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped_resource);

            // Update data
            memcpy(mapped_resource.pData, data, byte_size);

            // Reenable GPU access to data
            device_context->Unmap(buffers[index], 0);
        }

        // Set constant buffer into shader
        switch (type)
        {
        case Vertex:
            device_context->VSSetConstantBuffers(index, 1, &buffers[index]);
            break;
        case Pixel:
            device_context->PSSetConstantBuffers(index, 1, &buffers[index]);
            break;
        }
    }

    /* VisualAttribute Instance Definitions */
    // Constructor:
    // Initializes attribute and subscribes it to be rendered
    VisualAttribute::VisualAttribute(Object* _object)
    {
        // Assign object reference
        object = _object;

        // Subscribe to attributes to be rendered
        attributes.push_back(this);
    }

    // Destructor:
    // Unsubscribes the visual attribute from being rendered
    VisualAttribute::~VisualAttribute()
    {
        // TODO
    }

    

}
}