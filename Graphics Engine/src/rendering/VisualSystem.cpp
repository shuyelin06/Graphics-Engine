#include "VisualSystem.h"

#include <assert.h>
#include <d3d11_1.h>

#include "VisualDebug.h"

#include "datamodel/Object.h"

#define RGB(rgb) ((rgb) / 255.f)

// TODO: Debug drawing w/ instancing 
// device_context->DrawIndexedInstanced(num_indices, VisualDebug::points.size(), 0, 0, 1);

namespace Engine
{

namespace Graphics
{
    // Constructor
    // Saves the handle to the application window and initializes the
    // system's data structures
    VisualSystem::VisualSystem(HWND _window)
    {
        window = _window;

        // Components
        light_components = std::vector<LightComponent*>();
        asset_components = std::vector<AssetComponent*>();
        view_components = std::vector<ViewComponent*>();

        // Managers
        shaderManager = ShaderManager();

        device = NULL;
        device_context = NULL;
        swap_chain = NULL;
        render_target_view = NULL;
        depth_stencil = NULL;
    }

    // Initialize: 
    // Initializes Direct3D11 for use in rendering
    void VisualSystem::initialize()
    {
        

        assetManager = AssetManager();
        // assetManager.LoadAssetFromOBJ("data/", "cube-tex.obj", "Model");
        assetManager.LoadAssetFromOBJ("data/", "model.obj", "Model");

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

        shaderManager.initialize(device);
    }

    // Update:
    // Performs a render pass to render items to the screen.
    void VisualSystem::update()
    {
        // Clear the the screen color
        float color[4] = { RGB(158.f), RGB(218.f), RGB(255.f), 1.0f };
        device_context->ClearRenderTargetView(render_target_view, color);

        // Load per-frame
        VertexShader* vShader = shaderManager.getVertexShader(VSDefault);
        PixelShader* pShader = shaderManager.getPixelShader(PSDefault);

        LightComponent* light = light_components[0];
        
        light->setRenderTarget(this);
        vShader->getCBHandle(CB1)->clearData();
        light->loadViewData(this, vShader->getCBHandle(CB1));

        for (AssetComponent* asset_component : asset_components)
        {
            asset_component->beginLoading();

            vShader->getCBHandle(CB2)->clearData();
            int numIndices = asset_component->loadMeshData(device_context, vShader->getCBHandle(CB2), device);

            while (numIndices != -1)
            {
                vShader->bindShader(device, device_context);
                pShader->bindShader(device, device_context);
                device_context->DrawIndexed(numIndices, 0, 0);

                vShader->getCBHandle(CB2)->clearData();
                numIndices = asset_component->loadMeshData(device_context, vShader->getCBHandle(CB2), device);
            }
        }
        
        // Set render target
        device_context->OMSetRenderTargets(1, &render_target_view, depth_stencil);
        device_context->ClearDepthStencilView(depth_stencil, D3D11_CLEAR_DEPTH, 1.0f, 0);

        D3D11_VIEWPORT viewport = getViewport();
        device_context->RSSetViewports(1, &viewport);

        // Render Meshes
        vShader = shaderManager.getVertexShader(VSShadow);
        pShader = shaderManager.getPixelShader(PSShadow);

        ViewComponent* view = view_components[0];

        vShader->getCBHandle(CB1)->clearData();
        view->loadViewData(this, vShader->getCBHandle(CB1));
        pShader->getCBHandle(CB1)->clearData();
        light->bindShadowMap(this, 0, pShader->getCBHandle(CB1));

        for (AssetComponent* asset_component : asset_components)
        {
            asset_component->beginLoading();

            vShader->getCBHandle(CB2)->clearData();
            int numIndices = asset_component->loadMeshData(device_context, vShader->getCBHandle(CB2), device);

            while (numIndices != -1)
            {
                vShader->bindShader(device, device_context);
                pShader->bindShader(device, device_context);
                device_context->DrawIndexed(numIndices, 0, 0);

                vShader->getCBHandle(CB2)->clearData();
                numIndices = asset_component->loadMeshData(device_context, vShader->getCBHandle(CB2), device);
            }
        }

        // Present what we rendered to
        swap_chain->Present(1, 0);

        // Clear debug points
        VisualDebug::points.clear();
    }

    // GetDevice:
    // Returns the device
    ID3D11Device* VisualSystem::getDevice() const
        { return device; }

    // GetDeviceContext:
    // Returns the device context
    ID3D11DeviceContext* VisualSystem::getDeviceContext()
        { return device_context; }

    // GetRenderTargetView:
    // Returns the render target view
    ID3D11RenderTargetView* VisualSystem::getRenderTargetView() const
        { return render_target_view; }

    // GetDepthStencilView:
    // Returns the depth & stencil buffer
    ID3D11DepthStencilView* VisualSystem::getDepthStencil() const
        { return depth_stencil; }


    // GetViewport:
    // Returns the current viewport
    D3D11_VIEWPORT VisualSystem::getViewport() const
    {
        // Get the window rectangle
        RECT winRect;
        GetClientRect(window, &winRect);

        // Generate viewport description
        D3D11_VIEWPORT viewport = {
            0.0f, 0.0f,
            (float) (winRect.right - winRect.left),
            (float) (winRect.bottom - winRect.top),
            0.0f,
            1.0f
        };

        // Set depth for the depth buffer
        viewport.MinDepth = 0;
        viewport.MaxDepth = 1;

        // Return
        return viewport;
    }

    // GetActiveCamera:
    // Returns the currently active camera in the scene.
    ViewComponent* VisualSystem::getActiveView()
    {
        // If there are no camera components, return nullptr.
        if (view_components.size() == 0)
            return nullptr;
        
        // Otherwise, (for now) just return the first one.
        return view_components[0];
    }
    
    // CreateTexture2D:
    // Creates a 2D texture for use in the rendering pipeline.
    ID3D11Texture2D* VisualSystem::CreateTexture2D(D3D11_BIND_FLAG bind_flag, int width, int height)
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
    
   
    /* 
    TODO: When can i remove this: 
    // Render:
    // Renders an entire scene, from its camera
    void VisualSystem::render(Scene& scene)
    {
        // Clear screen to be black


        // Clear Depth Buffer and Depth Stencil


        // Get main scene camera
        Camera& camera = scene.getCamera();

        // Get lights and objects
        vector<Light*>& lights = scene.getLights();
        vector<Object*>& objects = scene.getObjects();

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
            Matrix4 m_camera = camera.getCameraMatrix();

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
        VisualDebug::DrawPoint(Vector3(2.f, 3.f, 5.f), 5.f, Vector3(0.75f, 0.25f, 0.35f));
        VisualDebug::DrawPoint(Vector3(2.f, 3.f, 19.f), 10.f, Vector3(0.25f, 0.25f, 0.35f));
        VisualDebug::DrawPoint(Vector3(2.f, 3.f, 0.f), 5.f, Vector3(0.35f, 0.55f, 0.35f));

        Mesh* cube = Mesh::GetMesh("CubeDebug");
        {
            Matrix4 m_worldToCamera = camera.getTransform().transformMatrix().inverse();
            Matrix4 m_camera = camera.getCameraMatrix();
            Matrix4 m_mat = m_worldToCamera * m_camera;
            BindVSData(0, m_mat.getRawData(), sizeof(Matrix4));
        }
        BindVSData(1, VisualDebug::points.data(), VisualDebug::points.size() * sizeof(PointData));
        renderMesh(*cube, id, scene, "DebugPoint", true);


        swap_chain->Present(1, 0);


        // Clearing debug point vector
        VisualDebug::points.clear();
    }

    // TraverseSceneGraph:
    // Recursively traverses a SceneGraph and renders
    // all renderable objects within this graph.
    void VisualSystem::traverseSceneGraph(Scene& scene, Object* object, Matrix4& m_parent)
    {
        // Get Local -> World transform
        Matrix4 m_local = object->getTransform().transformMatrix() * m_parent;

        // If mesh exists, render the object with this transform 
        if (object->getMesh() != nullptr)
        {
            Camera& camera = scene.getCamera();

            // Bind transformation matrices to pipeline
            Matrix4 m_worldToCamera = camera.getTransform().transformMatrix().inverse();
            Matrix4 m_camera = camera.getCameraMatrix();

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
    void VisualSystem::renderMesh(Mesh& mesh, Matrix4& m_modelToWorld, Scene& scene, std::string shader_config, bool instancing)
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
                vertex_buffer = CreateBuffer(
                    D3D11_BIND_VERTEX_BUFFER,
                    (void*)vertices.data(),
                    sizeof(float) * vertices.size());
                index_buffer = CreateBuffer(
                    D3D11_BIND_INDEX_BUFFER,
                    (void*)indices.data(),
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

     
        // Define input layout
        device_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        device_context->IASetInputLayout(input_layouts[mesh.getVertexLayout()]);

 
        // Bind vertex shader
        device_context->VSSetShader(vertex_shaders[shader_config], NULL, 0);

        // Bind pixel shader
        device_context->PSSetShader(pixel_shaders[shader_config], NULL, 0);

        // Draw from our vertex buffer
        if (!instancing)
            device_context->DrawIndexed(num_indices, 0, 0);
        else
            
    }
    
    */

    // --- Component Handling ----
    // MeshComponent:
    // Allows the rendering of a mesh using the object's transform.
    // Meshes have material properties, which specify how they are to be rendered.
    AssetComponent* VisualSystem::bindAssetComponent(Datamodel::Object* object, std::string assetName)
    {
        // Create component
        Asset* asset = assetManager.getAsset(assetName);
        AssetComponent* component = new AssetComponent(object, this, asset);
        asset_components.push_back(component);

        // Register with object
        object->registerComponent<AssetComponent>(component);

        return component;
    }

    bool VisualSystem::removeAssetComponent(AssetComponent* component)
    {
        auto index = std::find(asset_components.begin(), asset_components.end(), component);

        if (index != asset_components.end())
        {
            asset_components.erase(index);
            return true;
        }
        else
            return false;
    }

    // ViewComponent:
    // Contains data needed to render a scene from a particular view.
    // This view can be used as a camera, or used to render to a texture
    // (for techniques like shadowmapping)
    ViewComponent* VisualSystem::bindViewComponent(Datamodel::Object* object)
    {
        // Create component
        ViewComponent* component = new ViewComponent(object, this);
        view_components.push_back(component);

        // Register with object
        object->registerComponent<ViewComponent>(component);

        return component;
    }

    bool VisualSystem::removeViewComponent(ViewComponent* component)
    {
        auto index = std::find(view_components.begin(), view_components.end(), component);

        if (index != view_components.end())
        {
            view_components.erase(index);
            return true;
        }
        else
            return false;
    }

    // LightComponent:
    // Used to convert an object to a light source with shadows.
    LightComponent* VisualSystem::bindLightComponent(Datamodel::Object* object)
    {
        // Create component
        LightComponent* component = new LightComponent(object, this);
        light_components.push_back(component);

        // Register with object
        object->registerComponent<LightComponent>(component);

        return component;
    }
    
    bool VisualSystem::removeLightComponent(LightComponent* component)
    {
        auto index = std::find(light_components.begin(), light_components.end(), component);

        if (index != light_components.end())
        {
            light_components.erase(index);
            return true;
        }
        else
            return false;
    }

}
}