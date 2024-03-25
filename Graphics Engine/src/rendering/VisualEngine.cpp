#include "VisualEngine.h"

#include <assert.h>
#include <d3d11_1.h>

#include "datamodel/Scene.h"
#include "datamodel/Object.h"
#include "datamodel/Camera.h"

namespace Engine
{

namespace Graphics
{
    /* --- Constructor --- */
    // Saves the handle to the application window
    VisualEngine::VisualEngine()
    {
        window = nullptr;

        device = NULL;
        device_context = NULL;
        swap_chain = NULL;

        render_target_view = NULL;
        depth_stencil = NULL;
    }

    /* --- Initialize --- */
    // Initializes Direct3D11 
    void VisualEngine::initialize(HWND _window)
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
        D3D11_INPUT_ELEMENT_DESC input_desc[] = {
            { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, sizeof(float) * 3, D3D11_INPUT_PER_VERTEX_DATA, 0} // Option 12 appends
        };
        create_vertex_shader(L"src/shaders/shader.hlsl", "vs_main", input_desc, ARRAYSIZE(input_desc));

        // Create Pixel Shader
        create_pixel_shader(L"src/shaders/shader.hlsl", "ps_main");
    }

    /* --- Rendering --- */
    // Prepare:
    // Prepares the graphics engine for a draw call, by clearing the
    // screen and depth buffer (for depth testing)
    void VisualEngine::prepare()
    {
        // Clear screen to be blac
        float color[4] = { 0, 0, 0, 1.0f };
        device_context->ClearRenderTargetView(render_target_view, color);

        // Clear depth stencil
        device_context->ClearDepthStencilView(depth_stencil, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0.0f);
    }

    // Render:
    // Renders an entire scene. Should be called once per frame.
    void VisualEngine::render(Scene* scene)
    {
        // Prepare the engine for rendering 
        prepare();

        // Draw all objects

        scene->cameras;
    }

    // Runs a draw call on an object
    void VisualEngine::drawObject(Datamodel::Camera* camera, Datamodel::Object* object)
    {
        // Obtain reference to object's mesh
        Mesh* mesh = object->mesh;

        // Obtain transformation matrices
        Matrix4 local_to_world = localToWorldMatrix(object);
        Matrix4 world_to_camera = localToWorldMatrix(camera).inverse();
        Matrix4 camera_to_project = projectionMatrix(camera);

        // Multiply to obtain transform matrix to pass to shader
        Matrix4 transform = local_to_world * world_to_camera * camera_to_project;
        Matrix4 rotate = object->rotationMatrix();

        // Bind transform matrix to the vertex shader
        bind_vs_data(0, transform.getRawData(), sizeof(float) * 16);
        bind_vs_data(1, rotate.getRawData(), sizeof(float) * 16);

        // Prepare mesh's vertex and index buffers for rendering
        ID3D11Buffer* vertex_buffer; // Vertex Buffer
        ID3D11Buffer* index_buffer;  // Index Buffer 

        // Bytes between each vertex 
        UINT vertex_stride = Mesh::VertexLayoutSize(mesh->vertex_layout) * sizeof(float);
        // Offset into the vertex buffer to start reading from 
        UINT vertex_offset = 0;
        // Number of vertices
        UINT vertex_count = mesh->vertices.size();
        // Number of indices
        UINT num_indices = mesh->indices.size();

        {
            // Get the mesh's vertex and index vectors
            const vector<float> vertices = mesh->vertices;
            const vector<int> indices = mesh->indices;

            // Check if the vertex / index buffers have already been created
            // before. If they have, just use the already created resources
            if (mesh_cache.contains(mesh))
            {
                // Obtain buffers from cache
                MeshBuffers buffers = mesh_cache[mesh];

                // Use these buffers
                vertex_buffer = buffers.vertex_buffer;
                index_buffer = buffers.index_buffer;
            }
            // If they haven't, create these resources and add them to the cache
            // so we don't need to recreate them
            else
            {
                // Create new buffer resources
                vertex_buffer = create_buffer(
                                    D3D11_BIND_VERTEX_BUFFER, 
                                    (void *) vertices.data(), 
                                    sizeof(float) * vertices.size());
                index_buffer = create_buffer(
                                    D3D11_BIND_INDEX_BUFFER, 
                                    (void *) indices.data(), 
                                    sizeof(int) * mesh->indices.size());

                // Add buffer resources to cache
                mesh_cache[mesh] = { vertex_buffer, index_buffer };
            }
        }

        // Get shaders to render mesh with
        std::pair<ID3D11VertexShader*, ID3D11InputLayout*> vertex_shader = vertex_shaders[mesh->vertex_shader];
        ID3D11PixelShader* pixel_shader = pixel_shaders[mesh->pixel_shader];

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

        // Set min and max depth for viewpoint (for depth testing)
        viewport.MinDepth = 0;
        viewport.MaxDepth = 1;

        // Give rectangle to rasterizer state function
        device_context->RSSetViewports(1, &viewport);

        // Set output merger to use our render target and depth test
        device_context->OMSetRenderTargets(1, &render_target_view, depth_stencil);

        /* Configure Input Assembler */
        // Define input layout
        device_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        device_context->IASetInputLayout(vertex_shader.second);

        // Bind vertex and index buffers
        device_context->IASetVertexBuffers(0, 1, &vertex_buffer, &vertex_stride, &vertex_offset);
        device_context->IASetIndexBuffer(index_buffer, DXGI_FORMAT_R32_UINT, 0);

        /* Configure Shaders*/
        // Bind vertex shader
        device_context->VSSetShader(vertex_shader.first, NULL, 0);
        
        // Bind pixel shader
        device_context->PSSetShader(pixel_shader, NULL, 0);

        // Draw from our vertex buffer
        device_context->DrawIndexed(num_indices, 0, 0);
    }

    // Swaps the swapchain buffers, presenting drawn content
    // to the screen
    void VisualEngine::present()
    {
        swap_chain->Present(1, 0);
    }

    /* --- Buffer Creation --- */
    // Creates a generic buffer that can be used throughout our graphics pipeline. 
    // Uses include but are not limited to:
    // 1) Constant Buffers (without resource renaming)
    // 2) Vertex Buffers
    // 3) Index Buffers 
    ID3D11Buffer* VisualEngine::create_buffer(D3D11_BIND_FLAG bind_flag, void* data, int byte_size)
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
    
    // Bind_Data
    // Uses dynamic resource renaming to fairly efficiently bind data to the vertex / pixel
    // shader constant registers, for use in the shaders
    void VisualEngine::bind_vs_data(unsigned int index, void* data, int byte_size)
        { bind_data(Vertex, index, data, byte_size); }

    void VisualEngine::bind_ps_data(unsigned int index, void* data, int byte_size)
        { bind_data(Pixel, index, data, byte_size); }

    void VisualEngine::bind_data(Shader_Type type, unsigned int index, void* data, int byte_size)
    {
        // Get vertex or pixel buffers depending on what's requested
        std::vector<ID3D11Buffer*> buffers = (type == Vertex) ? vs_constant_buffers : ps_constant_buffers;

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

    /* --- Shader Creation --- */
    // The below functions can be used to compile shaders
    // for the graphics engine.
    
    // Compiles a shader blob
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

    /* Transformation Matrices */
    // The below methods are helper methods that generate
    // Matrix4 transformations for a given object

    // LocalToWorldMatrix:
    // Given an object, returns the transformation matrix that will
    // translate points in its local space to the world space
    Matrix4 VisualEngine::localToWorldMatrix(const Object* object)
    {
        // return object->localToWorldMatrix();
        // Generate the local transformation matrices
        Matrix4 m_scale = scaleMatrix(object->scale);
        Matrix4 m_rotation = rotationMatrix(object->rotation);
        Matrix4 m_translation = translationMatrix(object->position_local);

        // Obtain object's parent transformation matrix
        const Object* parent = object->parent;
        Matrix4 m_parent = parent == nullptr ? Matrix4::identity() : localToWorldMatrix(parent);

        // Build final matrix
        // Left matrix gets precedence, as we are performing row-major multiplication
        return m_scale * m_rotation * m_translation * m_parent;
    }

    // ProjectionMatrix: 
    // Given a camera wtih FOV, Z_Near, and Z_Far, generates its
    // projection matrix
    #define ASPECT_RATIO (1920.f / 1080.f)
    Matrix4 VisualEngine::projectionMatrix(const Camera* camera)
    {
        // Get camera's local variables
        float fov = camera->fov;
        float z_near = camera->z_near;
        float z_far = camera->z_far;

        // Generate matrix
        Matrix4 local_to_project = Matrix4();

        float fov_factor = cosf(fov / 2.f) / sinf(fov / 2.f);

        local_to_project[0][0] = fov_factor / ASPECT_RATIO;
        local_to_project[1][1] = fov_factor;
        local_to_project[2][2] = z_far / (z_far - z_near);
        local_to_project[2][3] = 1;
        local_to_project[3][2] = (z_near * z_far) / (z_near - z_far);

        return local_to_project;


    }

    // ScaleMatrix:
    // Given an x,y,z scale, returns the corresponding scale 
    // transformation matrix
    Matrix4 VisualEngine::scaleMatrix(const Vector3 scale)
    {
        return Matrix4(scale.x, 0, 0, 0,
            0, scale.y, 0, 0,
            0, 0, scale.z, 0,
            0, 0, 0, 1);
    }

    // RotationMatrix:
    // Given a roll, pitch, yaw, returns the corresponding rotation
    // transformation matrix
    Matrix4 VisualEngine::rotationMatrix(const Vector3 rotation)
    {
        // Cache values to avoid recalculating sine and cosine a lot
        float cos_cache = 0;
        float sin_cache = 0;

        // Rotation about the x-axis (roll)
        cos_cache = cosf(rotation.x);
        sin_cache = sin(rotation.x);
        Matrix4 roll = Matrix4(
            1, 0, 0, 0,
            0, cos_cache, sin_cache, 0,
            0, -sin_cache, cos_cache, 0,
            0, 0, 0, 1
        );

        // Rotation about the y-axis (pitch)
        cos_cache = cosf(rotation.y);
        sin_cache = sin(rotation.y);
        Matrix4 pitch = Matrix4(
            cos_cache, 0, -sin_cache, 0,
            0, 1, 0, 0,
            sin_cache, 0, cos_cache, 0,
            0, 0, 0, 1
        );

        // Rotation about the z-axis (yaw)
        cos_cache = cosf(rotation.z);
        sin_cache = sin(rotation.z);
        Matrix4 yaw = Matrix4(
            cos_cache, sin_cache, 0, 0,
            -sin_cache, cos_cache, 0, 0,
            0, 0, 1, 0,
            0, 0, 0, 1
        );

        return roll * pitch * yaw;
    }

    // TranslationMatrix:
    // Given a x,y,z position locally, returns the corresponding
    // translation matrix
    Matrix4 VisualEngine::translationMatrix(const Vector3 translation)
    {
        return Matrix4(
            1, 0, 0, 0,
            0, 1, 0, 0,
            0, 0, 1, 0,
            translation.x, translation.y, translation.z, 1);
    }


}
}