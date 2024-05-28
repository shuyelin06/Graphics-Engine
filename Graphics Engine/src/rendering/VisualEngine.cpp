#include "VisualEngine.h"

#include <assert.h>
#include <d3d11_1.h>

#include "ShaderData.h"

#include "datamodel/Scene.h"
#include "datamodel/Object.h"
#include "datamodel/Camera.h"

#define RGB(rgb) ((rgb) / 255.f)

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

        // Create resources
        input_layouts = map<char, ID3D11InputLayout*>();

        vertex_shaders = map<string, ID3D11VertexShader*>();
        pixel_shaders = map<string, ID3D11PixelShader*>();

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
        ID3D11Texture2D* depth_texture = CreateTexture2D(D3D11_BIND_DEPTH_STENCIL, width, height);

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
        // Default Renderer
        vertex_shaders["Default"] = CreateVertexShader(L"src/rendering/shaders/VertexShader.hlsl", "vs_main", XYZ | NORMAL);
        pixel_shaders["Default"] = CreatePixelShader(L"src/rendering/shaders/PixelShader.hlsl", "ps_main");

        // Debug Point Renderer (with Instancing)
        debug_points.push_back({ {2.f, 3.f, 5.f}, 0.f, {0.75f, 0.25f, 0.35f}, 0.f, 5.f, {} });
        debug_points.push_back({ {2.f, 3.f, 19.f}, 0.f, {0.25f, 0.25f, 0.35f}, 0.f, 10.f, {} });
        debug_points.push_back({ {2.f, 3.f, 0.f}, 0.f, {0.35f, 0.55f, 0.35f}, 0.f, 5.f, {} });

        vertex_shaders["DebugPoint"] = CreateVertexShader(L"src/rendering/shaders/PointRenderer.hlsl", "vs_main", XYZ | INSTANCING);
        pixel_shaders["DebugPoint"] = CreatePixelShader(L"src/rendering/shaders/PointRenderer.hlsl", "ps_main");
    }

    /* --- Rendering --- */
    // Render:
    // Renders an entire scene, from its camera
    void VisualEngine::render(Scene& scene)
    {
        /* Rendering Preparation */
        // Clear screen to be black
        float color[4] = { RGB(158.f), RGB(218.f), RGB(255.f), 1.0f};
        device_context->ClearRenderTargetView(render_target_view, color);

        // Clear Depth Buffer and Depth Stencil
        device_context->ClearDepthStencilView(depth_stencil, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0.0f);

        /* Rendering */
        // Get main scene camera
        Camera& camera = scene.getCamera();

        // Get lights and objects
        vector<Light*>& lights = scene.getLights();
        vector<Object*>& objects = scene.getObjects();

        /* Bind Lights */
        {
            #define NUM_LIGHTS 10

            vector<LightData> light_data;
            light_data.resize(NUM_LIGHTS);

            for (int i = 0; i < lights.size(); i++)
            {
                Light& light = *(lights[i]);

                Transform& transform = light.getTransform();

                Matrix4 m_transform = light.localToWorldMatrix();
                Vector3 world_position = (Vector4(0, 0, 0, 1) * m_transform).toVector3();

                light_data[i].position = world_position;

                BindPSData(0, light_data.data(), light_data.size() * sizeof(LightData));
            }

        }

        // Iterate through SceneGraph to render every object
        Matrix4 m_transform = Matrix4::identity();

        for (Object* object : objects) 
            traverseSceneGraph(scene, object, m_transform);
        
        // Render the Terrain
        Matrix4 id = Matrix4::identity();
        {
            Camera& camera = scene.getCamera();

            // Bind transformation matrices to pipeline
            Matrix4 m_worldToCamera = camera.getTransform().transformMatrix().inverse();
            Matrix4 m_camera = camera.cameraMatrix();

            // Create & populate struct with data
            TransformData transform_data;

            // Model -> World Space Transform (Vertices)
            transform_data.m_modelToWorld = id;
            // Model -> Camera Space Transform
            transform_data.m_worldToCamera = m_worldToCamera * m_camera;
            // Model -> World Space Transform (Normals)
            transform_data.m_normalTransform = id.inverse().tranpose();

            // Bind to vertex shader, into constant buffer @index 0
            BindVSData(0, &transform_data, sizeof(TransformData));
        }
        renderMesh(scene.getTerrain().getMesh(), id, scene, "Default", false);

        // Render Debug Points
        Mesh* cube = Mesh::GetMesh("CubeDebug");
        {
            Matrix4 m_worldToCamera = camera.getTransform().transformMatrix().inverse();
            Matrix4 m_camera = camera.cameraMatrix();
            Matrix4 m_mat = m_worldToCamera * m_camera;
            BindVSData(0, m_mat.getRawData(), sizeof(Matrix4));
        }
        BindVSData(1, debug_points.data(), debug_points.size() * sizeof(PointData));
        renderMesh(*cube, id, scene, "DebugPoint", true);

        /* Presenting */
        swap_chain->Present(1, 0);
    }

    // TraverseSceneGraph:
    // Recursively traverses a SceneGraph and renders
    // all renderable objects within this graph.
    void VisualEngine::traverseSceneGraph(Scene& scene, Object* object, Matrix4& m_parent)
    {
        // Get Local -> World transform
        Matrix4 m_local = object->getTransform().transformMatrix() * m_parent;

        // If mesh exists, render the object with this transform 
        if (object->getMesh() != nullptr)
        {
            Camera& camera = scene.getCamera();

            // Bind transformation matrices to pipeline
            Matrix4 m_worldToCamera = camera.getTransform().transformMatrix().inverse();
            Matrix4 m_camera = camera.cameraMatrix();

            // Create & populate struct with data
            TransformData transform_data;

            // Model -> World Space Transform (Vertices)
            transform_data.m_modelToWorld = m_local;
            // Model -> Camera Space Transform
            transform_data.m_worldToCamera = m_worldToCamera * m_camera;
            // Model -> World Space Transform (Normals)
            transform_data.m_normalTransform = m_local.inverse().tranpose();

            // Bind to vertex shader, into constant buffer @index 0
            BindVSData(0, &transform_data, sizeof(TransformData));

            renderMesh(*(object->getMesh()), m_local, scene, "Default", false);
        }
            
        
        // Recursively traverse the SceneGraph for the object's children
        for (Object* child : object->getChildren())
            traverseSceneGraph(scene, child, m_local);
    }

    // DrawObject:
    // Given a renderable mesh, renders it within the scene
    void VisualEngine::renderMesh(Mesh& mesh, Matrix4& m_modelToWorld, Scene& scene, std::string shader_config, bool instancing)
    {
        // If mesh has nothing, do nothing
        if (mesh.getIndexBuffer().size() == 0 || mesh.getVertexBuffer().size() == 0)
            return;

        // Bind Mesh Vertex & Index Buffers
        // Get the mesh's vertex and index vectors
        const vector<float>& vertices = mesh.getVertexBuffer();
        const vector<int>& indices = mesh.getIndexBuffer();

        // Number of indices
        UINT num_indices = indices.size();

        // Generate vertex and index buffers. If they have already been
        // generated before, just get them.
        ID3D11Buffer* vertex_buffer = NULL;
        ID3D11Buffer* index_buffer = NULL;
        {
            // Memory address of mesh
            Mesh* mem_addr = &mesh;

            // Bytes between each vertex 
            UINT vertex_stride = Mesh::VertexLayoutSize(mesh.getVertexLayout()) * sizeof(float);
            // Offset into the vertex buffer to start reading from 
            UINT vertex_offset = 0;

            // Check if the vertex / index buffers have already been created
            // before. If they have, just use the already created resources
            if (mesh_cache.contains(mem_addr))
            {
                // Obtain buffers from cache
                MeshBuffers buffers = mesh_cache[mem_addr];

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
                    (void*) vertices.data(),
                    sizeof(float) * vertices.size());
                index_buffer = create_buffer(
                    D3D11_BIND_INDEX_BUFFER,
                    (void*) indices.data(),
                    sizeof(int) * indices.size());

                // Add buffer resources to cache
                mesh_cache[mem_addr] = { vertex_buffer, index_buffer };
            }

            // Bind vertex and index buffers
            device_context->IASetVertexBuffers(0, 1, &vertex_buffer, &vertex_stride, &vertex_offset);
            device_context->IASetIndexBuffer(index_buffer, DXGI_FORMAT_R32_UINT, 0);
        }

        // Perform a Draw Call
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
        device_context->IASetInputLayout(input_layouts[mesh.getVertexLayout()]);

        /* Configure Shaders*/
        // Bind vertex shader
        device_context->VSSetShader(vertex_shaders[shader_config], NULL, 0);

        // Bind pixel shader
        device_context->PSSetShader(pixel_shaders[shader_config], NULL, 0);

        // Draw from our vertex buffer
        if (!instancing)
            device_context->DrawIndexed(num_indices, 0, 0);
        else
            device_context->DrawIndexedInstanced(num_indices, debug_points.size(), 0, 0, 1);
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
    
    // CreateTexture2D:
    // Creates a 2D texture for use in the rendering pipeline.
    ID3D11Texture2D* VisualEngine::CreateTexture2D(D3D11_BIND_FLAG bind_flag, int width, int height)
    {
        ID3D11Texture2D* texture = NULL;

        // Create texture description
        D3D11_TEXTURE2D_DESC desc_texture = {};
        desc_texture.Width = width;
        desc_texture.Height = height;
        desc_texture.MipLevels = 1;
        desc_texture.ArraySize = 1;
        desc_texture.MipLevels = 1;
        desc_texture.ArraySize = 1;
        desc_texture.SampleDesc.Count = 1;
        desc_texture.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
        desc_texture.BindFlags = bind_flag;

        // Create 2D texture 
        HRESULT result = device->CreateTexture2D(&desc_texture, NULL, &texture);

        // Check for failure
        if (result != S_OK)
            assert(false);

        return texture;
    }

    // Bind_Data
    // Uses dynamic resource renaming to fairly efficiently bind data to the vertex / pixel
    // shader constant registers, for use in the shaders
    void VisualEngine::BindVSData(unsigned int index, void* data, int byte_size)
        { BindData(Vertex, index, data, byte_size); }

    void VisualEngine::BindPSData(unsigned int index, void* data, int byte_size)
        { BindData(Pixel, index, data, byte_size); }

    void VisualEngine::BindData(Shader_Type type, unsigned int index, void* data, int byte_size)
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
    static ID3DBlob* CompileShaderBlob(Shader_Type type, const wchar_t* file, const char* entry)
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

    // Creates a vertex shader and adds it to the array of vertex shaders
    ID3D11VertexShader* VisualEngine::CreateVertexShader(const wchar_t* filename, const char* entrypoint, char layout)
    {
        // Obtain shader blob
        ID3DBlob* shader_blob = CompileShaderBlob(Vertex, filename, entrypoint);

        // Create input layout for vertex shader, or use it if it already exists
        ID3D11InputLayout* input_layout = NULL;

        if (input_layouts.contains(layout))
        {
            input_layout = input_layouts.at(layout);
        }
        else {
            D3D11_INPUT_ELEMENT_DESC input_desc[10];
            int input_desc_size = 0;

            // Supported input configurations with instancing
            if ((layout & INSTANCING) == INSTANCING)
            {
                // XYZ Position
                if (layout == -127)
                {
                    // POSITION: float3, SV_InstanceID: uint
                    input_desc[0] = { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 };
                    input_desc[1] = { "SV_InstanceID", 0, DXGI_FORMAT_R32_UINT, 0, sizeof(float) * 3, D3D11_INPUT_PER_VERTEX_DATA, 0 };
                    input_desc_size = 2;
                }

            }
            // Supported input configurations without instancing
            else
            {
                // XYZ Position and Normal Vectors
                if (layout == (XYZ | NORMAL))
                {
                    // POSITION: float3, NORMAL: float3
                    input_desc[0] = { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 };
                    input_desc[1] = { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, sizeof(float) * 3, D3D11_INPUT_PER_VERTEX_DATA, 0 };
                    input_desc_size = 2;
                }
            }
            

            // Create input layout resource
            device->CreateInputLayout(
                input_desc,
                input_desc_size,
                shader_blob->GetBufferPointer(),
                shader_blob->GetBufferSize(),
                &input_layout
            );

            input_layouts[layout] = input_layout;

            // Check for success
            assert(input_layout != NULL);
        }

        // Create vertex shader
        ID3D11VertexShader* vertex_shader = NULL;

        device->CreateVertexShader(
            shader_blob->GetBufferPointer(),
            shader_blob->GetBufferSize(),
            NULL,
            &vertex_shader
        );

        return vertex_shader;
    }

    // Creates a pixel shader and adds it to the array of pixel shaders
    ID3D11PixelShader* VisualEngine::CreatePixelShader(const wchar_t* filename, const char* entrypoint)
    {
        // Obtain shader blob
        ID3DBlob* shader_blob = CompileShaderBlob(Pixel, filename, entrypoint);

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
        return pixel_shader;
    }

}
}