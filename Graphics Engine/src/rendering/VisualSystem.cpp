#include "VisualSystem.h"

#include <assert.h>
#include <d3d11_1.h>

#include "VisualDebug.h"
#include "core/Frustum.h"
#include "datamodel/Object.h"

namespace Engine {

namespace Graphics {
// Constructor
// Initializes the VisualSystem
VisualSystem::VisualSystem(HWND window) {
    device = NULL;
    context = NULL;

    resource_manager = NULL;
    pipeline_manager = NULL;

    swap_chain = NULL;
    screen_target = NULL;

    camera = nullptr;
    terrain = nullptr;

    time_elapsed = 0.f;

    HRESULT result;

    // Get window width and height
    RECT rect;
    GetClientRect(window, &rect);
    const UINT width = rect.right - rect.left;
    const UINT height = rect.bottom - rect.top;

    // Create my swap chain and set up its buffer as a render target. This is
    // what will be displayed on our screen
    initializeScreenTarget(window, width, height);

    // Create another render target and depth stencil. We will render to this as
    // an intermediate target so that we can apply post processing effects
    initializeRenderTarget(width, height);

    // Initialize my full screen quad. This will be used for post-processing
    // effects.
    initializeFullscreenQuad();

    // Initialize each of my managers with the resources they need
    initializeManagers();

    // Initialize My Components
    initializeComponents();

#if defined(_DEBUG)
    imGuiInitialize(window);
    imGuiPrepare(); // Pre-Prepare a Frame
#endif
}

// InitializeScreenTarget:
// Create swap chain, device, and context.
// The swap chain is responsible for swapping between textures
// for rendering, and the device + context give us interfaces to work with the
// GPU.
void VisualSystem::initializeScreenTarget(HWND window, UINT width,
                                          UINT height) {
    HRESULT result;

    // Create my swap chain. This will let me swap between textures for
    // rendering, so the user doesn't see the next frame while it's being
    // rendered.
    DXGI_SWAP_CHAIN_DESC swap_chain_descriptor = {0};

    swap_chain_descriptor.BufferDesc.RefreshRate.Numerator = 0;
    swap_chain_descriptor.BufferDesc.RefreshRate.Denominator = 1;
    swap_chain_descriptor.BufferDesc.Width = width;
    swap_chain_descriptor.BufferDesc.Height = height;
    swap_chain_descriptor.BufferDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    swap_chain_descriptor.SampleDesc.Count = 1;
    swap_chain_descriptor.SampleDesc.Quality = 0;
    swap_chain_descriptor.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swap_chain_descriptor.BufferCount = 1; // # Back Buffers
    swap_chain_descriptor.OutputWindow = window;
    swap_chain_descriptor.Windowed = true; // Displaying to a Window

    D3D_FEATURE_LEVEL feature_level; // Stores the GPU functionality
    result = D3D11CreateDeviceAndSwapChain(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL,
                                           0, // Flags
                                           NULL, 0, D3D11_SDK_VERSION,
                                           &swap_chain_descriptor, &swap_chain,
                                           &device, &feature_level, &context);

    assert(S_OK == result && swap_chain && device && context);

    // Create my render target with the swap chain's frame buffer. This
    // will store my output image.
    screen_target = new Texture(width, height);

    result = swap_chain->GetBuffer(0, __uuidof(ID3D11Texture2D),
                                   (void**)&screen_target->texture);
    assert(SUCCEEDED(result));

    result = device->CreateRenderTargetView(screen_target->texture, 0,
                                            &screen_target->target_view);
    assert(SUCCEEDED(result));

    screen_target->texture->Release(); // Free frame buffer (no longer needed)

    // Create my viewport
    viewport = {0.0f, 0.0f, (float)width, (float)height, 0.0f, 1.0f};
}

// InitializeRenderTarget:
// Initializes a render target and depth stencil we will use as an intermediate
// render target for post processing effects
void VisualSystem::initializeRenderTarget(UINT width, UINT height) {
    HRESULT result;

    // Create 2 render targets. We will ping pong between
    // these two render targets during post processing.
    render_target_dest = new Texture(width, height);
    render_target_src = new Texture(width, height);

    D3D11_TEXTURE2D_DESC tex_desc = {};
    tex_desc.Width = width;
    tex_desc.Height = height;
    tex_desc.MipLevels = 1;
    tex_desc.ArraySize = 1;
    tex_desc.MipLevels = 1;
    tex_desc.ArraySize = 1;
    tex_desc.SampleDesc.Count = 1;
    tex_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
    tex_desc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;

    // Create my texture resources
    result =
        device->CreateTexture2D(&tex_desc, NULL, &render_target_src->texture);
    assert(SUCCEEDED(result));
    result =
        device->CreateTexture2D(&tex_desc, NULL, &render_target_dest->texture);
    assert(SUCCEEDED(result));

    // Create render target views
    result = device->CreateRenderTargetView(render_target_dest->texture, 0,
                                            &render_target_dest->target_view);
    assert(SUCCEEDED(result));
    result = device->CreateRenderTargetView(render_target_src->texture, 0,
                                            &render_target_src->target_view);
    assert(SUCCEEDED(result));

    // Create shader views so we can access them in the GPU
    {
        D3D11_SHADER_RESOURCE_VIEW_DESC resource_view_desc = {};
        resource_view_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
        resource_view_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
        resource_view_desc.Texture2D.MostDetailedMip = 0;
        resource_view_desc.Texture2D.MipLevels = 1;

        result = device->CreateShaderResourceView(
            render_target_dest->texture, &resource_view_desc,
            &render_target_dest->shader_view);
        assert(SUCCEEDED(result));
        result = device->CreateShaderResourceView(
            render_target_src->texture, &resource_view_desc,
            &render_target_src->shader_view);
        assert(SUCCEEDED(result));
    }

    // Create another texture as a depth stencil, for z-testing
    // Create my depth stencil which will be used for z-tests
    // 24 Bits for Depth, 8 Bits for Stencil
    depth_stencil = new Texture(width, height);

    tex_desc.Width = width;
    tex_desc.Height = height;
    tex_desc.MipLevels = 1;
    tex_desc.ArraySize = 1;
    tex_desc.MipLevels = 1;
    tex_desc.ArraySize = 1;
    tex_desc.SampleDesc.Count = 1;
    tex_desc.Format = DXGI_FORMAT_R24G8_TYPELESS;
    tex_desc.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;

    result = device->CreateTexture2D(&tex_desc, NULL, &depth_stencil->texture);
    assert(SUCCEEDED(result));

    // Create my depth view
    D3D11_DEPTH_STENCIL_VIEW_DESC desc_stencil = {};
    desc_stencil.Format =
        DXGI_FORMAT_D24_UNORM_S8_UINT; // Same format as texture
    desc_stencil.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DMS;

    result = device->CreateDepthStencilView(
        depth_stencil->texture, &desc_stencil, &depth_stencil->depth_view);
    assert(SUCCEEDED(result));

    // Create my shader view for the depth texture
    {
        D3D11_SHADER_RESOURCE_VIEW_DESC resource_view_desc = {};
        resource_view_desc.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
        resource_view_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
        resource_view_desc.Texture2D.MostDetailedMip = 0;
        resource_view_desc.Texture2D.MipLevels = 1;

        result = device->CreateShaderResourceView(depth_stencil->texture,
                                                  &resource_view_desc,
                                                  &depth_stencil->shader_view);
        assert(SUCCEEDED(result));
    }

    // Create a blend state
    D3D11_BLEND_DESC blend_desc = {};
    blend_desc.RenderTarget[0].BlendEnable = TRUE;

    blend_desc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
    blend_desc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
    blend_desc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;

    blend_desc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_SRC_ALPHA;
    blend_desc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA;
    blend_desc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_MAX;

    blend_desc.RenderTarget[0].RenderTargetWriteMask =
        D3D11_COLOR_WRITE_ENABLE_ALL;
    result = device->CreateBlendState(&blend_desc, &blend_state);
    assert(SUCCEEDED(result));
}

// InitializeFullscreenQuad:
// Creates a quad that covers the entire screen, so we can use the pixel shader
// for post processing effects
void VisualSystem::initializeFullscreenQuad() {
    const Vector4 fullscreen_quad[6] = {
        // First Triangle
        Vector4(-1, -1, 0, 1), Vector4(-1, 1, 0, 1), Vector4(1, 1, 0, 1),
        // Second Triangle
        Vector4(-1, -1, 0, 1), Vector4(1, 1, 0, 1), Vector4(1, -1, 0, 1)};

    D3D11_BUFFER_DESC buffer_desc = {};
    buffer_desc.ByteWidth = sizeof(fullscreen_quad);
    buffer_desc.Usage = D3D11_USAGE_DEFAULT;
    buffer_desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;

    D3D11_SUBRESOURCE_DATA sr_data = {};
    sr_data.pSysMem = (void*)fullscreen_quad;

    device->CreateBuffer(&buffer_desc, &sr_data, &postprocess_quad);
}

// InitializeManagers:
// Initializes different managers that the visual system uses.
void VisualSystem::initializeManagers() {
    HRESULT result;

    resource_manager = new ResourceManager(device, context);
    resource_manager->initializeResources();

    pipeline_manager = new PipelineManager(device, context);

    // --- Light Manager ---
    constexpr int ATLAS_SIZE = 4096;
    Texture* atlas_texture = new Texture(ATLAS_SIZE, ATLAS_SIZE);

    // Create my texture resource. This will have 24 Bits for R Channel (depth),
    // 8 Bits for G Channel (stencil). The resource will be able to be accessed
    // as a depth stencil and shader resource.
    D3D11_TEXTURE2D_DESC tex_desc = {};
    tex_desc.Width = ATLAS_SIZE;
    tex_desc.Height = ATLAS_SIZE;
    tex_desc.MipLevels = tex_desc.ArraySize = 1;
    tex_desc.Format = DXGI_FORMAT_R24G8_TYPELESS;
    tex_desc.SampleDesc.Count = 1;
    tex_desc.Usage = D3D11_USAGE_DEFAULT;
    tex_desc.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;
    tex_desc.CPUAccessFlags = 0;
    tex_desc.MiscFlags = 0;

    result = device->CreateTexture2D(&tex_desc, NULL, &atlas_texture->texture);
    assert(SUCCEEDED(result));

    // Initialize a depth stencil view, to allow the texture to be used as a
    // depth buffer. DXGI_FORMAT_D24_UNORM_S8_UINT specifies 24 bits for depth,
    // 8 bits for stencil
    D3D11_DEPTH_STENCIL_VIEW_DESC depth_stencil_view_desc = {};
    depth_stencil_view_desc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    depth_stencil_view_desc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
    depth_stencil_view_desc.Texture2D.MipSlice = 0;

    result = device->CreateDepthStencilView(atlas_texture->texture,
                                            &depth_stencil_view_desc,
                                            &atlas_texture->depth_view);
    assert(SUCCEEDED(result));

    // Initialize a shader resource view, so that the texture data
    // can be sampled in the shader.
    // DXGI_FORMAT_R24_UNORM_X8_TYPELESS specifies 24 bits in the R channel
    // UNORM (0.0f -> 1.0f), and 8 bits to be ignored
    D3D11_SHADER_RESOURCE_VIEW_DESC shader_resource_view_description = {};
    shader_resource_view_description.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
    shader_resource_view_description.ViewDimension =
        D3D11_SRV_DIMENSION_TEXTURE2D;
    shader_resource_view_description.Texture2D.MostDetailedMip = 0;
    shader_resource_view_description.Texture2D.MipLevels = 1;

    result = device->CreateShaderResourceView(atlas_texture->texture,
                                              &shader_resource_view_description,
                                              &atlas_texture->shader_view);
    assert(SUCCEEDED(result));

    light_manager = new LightManager(new TextureAtlas(atlas_texture));
}

// InitializeComponents:
// Register components of the visual system.
void VisualSystem::initializeComponents() {
    // ...
}

// Shutdown:
// Closes the visual system.
void VisualSystem::shutdown() {
#if defined(_DEBUG)
    imGuiShutdown();
#endif
}

// --- Component Bindings ---
CameraComponent* VisualSystem::bindCameraComponent(Object* object) {
    if (camera != nullptr)
        delete camera;
    CameraComponent* new_cam = new CameraComponent(object);
    object->bindComponent(new_cam);
    camera = new_cam;

    return camera;
}

AssetComponent*
VisualSystem::bindAssetComponent(Object* object,
                                 const std::string& asset_name) {
    Asset* asset = resource_manager->getAsset(asset_name);
    AssetComponent* asset_obj = new AssetComponent(object, asset);
    asset_components.newComponent(object, asset_obj);
    return asset_obj;
}

ShadowLightComponent* VisualSystem::bindLightComponent(Object* object) {
    ShadowLight* light = light_manager->createShadowLight(QUALITY_1);
    ShadowLightComponent* light_obj = new ShadowLightComponent(object, light);
    light_components.newComponent(object, light_obj);
    return light_obj;
}

VisualTerrain* VisualSystem::bindTerrain(Terrain* _terrain) {
    if (terrain != nullptr)
        delete terrain;
    terrain = new VisualTerrain(_terrain, device);

    return terrain;
}

// Render:
// Renders the entire scene to the screen.
void VisualSystem::render() {
#if defined(_DEBUG)
    gpu_timer.beginTimer("GPU Frametime");
    CPUTimer::BeginCPUTimer("CPU Frametime");
#endif

    // TEMP
    light_manager->updateTimeOfDay(15.f);

    // Clear the the screen color
    render_target_dest->clearAsRenderTarget(
        context, Color(173.f / 255.f, 216.f / 255.f, 230.f / 255.f));

    // Prepare the shadow maps
    performShadowPass(); //..

    // Render terrain
    performTerrainPass();
    // Render meshes
    performRenderPass();

    // Rendering is different depending on if we're below
    // or above the surface of the water
    const Vector3& cam_pos = camera->getPosition();
    if (cam_pos.y <= 100.f) {
        processUnderwater();
    } else {
        performWaterSurfacePass();
    }

    performLightFrustumPass();

#if defined(ENABLE_DEBUG_DRAWING)
    // Debug Functionality
    renderDebugPoints();
    renderDebugLines();
    VisualDebug::Clear();
#endif

#if defined(_DEBUG)
    gpu_timer.endTimer("GPU Frametime");
    CPUTimer::EndCPUTimer("CPU Frametime");
    imGuiFinish();
#endif

    renderFinish();

#if defined(_DEBUG)
    // Prepare for the next frame
    imGuiPrepare();
#endif
}

// RenderPrepare:
// Prepares the engine for rendering, by pulling all necessary
// data from the datamodel.
void VisualSystem::pullDatamodelData() {
#if defined(_DEBUG)
    CPUTimer::BeginCPUTimer("Render Prepare");
#endif

    // Pull my object data.
    // Remove invalid visual objects, and update them to pull
    // the datamodel data.
    if (camera != nullptr)
        camera->update();
    asset_components.cleanAndUpdate();
    light_components.cleanAndUpdate();

    // Pull my terrain meshes
    terrain->pullTerrainMeshes(context);

    // Prepare managers for data
    const Frustum cam_frustum = camera->frustum();
    light_manager->updateSunCascades(camera->frustum());

    light_manager->resetShadowCasters();

    for (const AssetComponent* object : asset_components.getComponents()) {
        const Asset* asset = object->getAsset();

        RenderableAsset renderable_asset;
        renderable_asset.asset = asset;
        renderable_asset.m_localToWorld = object->getObject()->getLocalMatrix();
        renderable_meshes.push_back(renderable_asset);

        const std::vector<Mesh*>& meshes = asset->getMeshes();
        for (const Mesh* mesh : meshes) {
            ShadowCaster shadowCaster;
            shadowCaster.mesh = mesh;
            shadowCaster.m_localToWorld = object->getObject()->getLocalMatrix();
            light_manager->addShadowCaster(shadowCaster);
        }
    }

    // Load it for shadows
    /*const std::vector<Mesh*>& terrain_meshes = terrain->getTerrainMeshes();
    for (Mesh* terrain_mesh : terrain_meshes) {
        if (terrain_mesh == nullptr)
            continue;

        ShadowCaster terrain_shadow;
        terrain_shadow.m_localToWorld = Matrix4::Identity();
        terrain_shadow.mesh = terrain_mesh;
    }*/

    // Cluster shadows
    light_manager->clusterShadowCasters();

#if defined(_DEBUG)
    CPUTimer::EndCPUTimer("Render Prepare");
#endif
}

// PerformShadowPass:
// Render the scene from each light's point of view, to populate
// its shadow map.
void VisualSystem::performShadowPass() {
#if defined(_DEBUG)
    gpu_timer.beginTimer("Shadow Pass");
    CPUTimer::BeginCPUTimer("Shadow Pass");
#endif

    pipeline_manager->bindVertexShader("ShadowMap");
    pipeline_manager->bindPixelShader("ShadowMap");

    const Texture* shadow_texture = light_manager->getAtlasTexture();
    context->ClearDepthStencilView(shadow_texture->depth_view,
                                   D3D11_CLEAR_DEPTH, 1.0f, 0);

    const std::vector<ShadowLight*>& lights = light_manager->getShadowLights();
    const std::vector<ShadowCluster>& clusters =
        light_manager->getShadowClusters();
    const std::vector<UINT>& casters = light_manager->getShadowClusterIndices();
    const std::vector<ShadowCaster>& shadow_casters =
        light_manager->getShadowCasters();

    for (const ShadowCluster& cluster : clusters) {
        ShadowLight* light = lights[cluster.light_index];

        // Load light view and projection matrix
        {
            CBHandle* vCB0 = pipeline_manager->getVertexCB(CB0);
            vCB0->clearData();

            const Matrix4& m_world_to_local = light->getWorldMatrix().inverse();
            vCB0->loadData(&m_world_to_local, FLOAT4X4);
            const Matrix4 m_local_to_frustum = light->getFrustumMatrix();
            vCB0->loadData(&m_local_to_frustum, FLOAT4X4);

            pipeline_manager->bindVertexCB(CB0);
        }

        // Set the light as the render target.
        const D3D11_VIEWPORT viewport = light->getShadowmapViewport().toD3D11();
        context->OMSetRenderTargets(0, nullptr, shadow_texture->depth_view);
        context->RSSetViewports(1, &viewport);

        // Run the shadow pass for every asset in the light's view
        for (int i = 0; i < cluster.caster_offset; i++) {
            const ShadowCaster caster =
                shadow_casters[casters[cluster.caster_start + i]];

            const Mesh* mesh = caster.mesh;
            const Matrix4& mLocalToWorld = caster.m_localToWorld;

            {
                CBHandle* vCB1 = pipeline_manager->getVertexCB(CB1);
                vCB1->clearData();
                vCB1->loadData(&mLocalToWorld, FLOAT4X4);
                pipeline_manager->bindVertexCB(CB1);
            }

            ID3D11Buffer* index_buffer = mesh->index_buffer;
            ID3D11Buffer* position_stream = mesh->vertex_streams[POSITION];
            UINT num_indices = mesh->triangle_count * 3;

            UINT vertexStride = sizeof(float) * 3;
            UINT vertexOffset = 0;

            context->IASetPrimitiveTopology(
                D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
            context->IASetVertexBuffers(POSITION, 1, &position_stream,
                                        &vertexStride, &vertexOffset);
            context->IASetIndexBuffer(index_buffer, DXGI_FORMAT_R32_UINT, 0);

            context->DrawIndexed(num_indices, 0, 0);
        }
    }

#if defined(_DEBUG)
    gpu_timer.endTimer("Shadow Pass");
    CPUTimer::EndCPUTimer("Shadow Pass");
#endif
}

void VisualSystem::performTerrainPass() {
#if defined(_DEBUG)
    gpu_timer.beginTimer("Terrain Pass");
    CPUTimer::BeginCPUTimer("Terrain Pass");
#endif

    pipeline_manager->bindVertexShader("Terrain");
    pipeline_manager->bindPixelShader("Terrain");

    // TEST
    bindActiveRenderTarget(EnableDepthStencil_TestAndWrite);
    context->ClearDepthStencilView(depth_stencil->depth_view, D3D11_CLEAR_DEPTH,
                                   1.0f, 0);

    // Vertex Constant Buffer 0:
    // Stores the camera view and projection matrices
    {
        CBHandle* vCB0 = pipeline_manager->getVertexCB(CB0);
        vCB0->clearData();

        const Matrix4 viewMatrix = camera->getWorldToCameraMatrix();
        vCB0->loadData(&viewMatrix, FLOAT4X4);
        const Matrix4 projectionMatrix = camera->getFrustumMatrix();
        vCB0->loadData(&projectionMatrix, FLOAT4X4);

        pipeline_manager->bindVertexCB(CB0);
    }

    // Pixel Constant Buffer 1: Light Data
    // Stores data that is needed for lighting / shadowing.
    {
        CBHandle* pCB1 = pipeline_manager->getPixelCB(CB1);
        pCB1->clearData();

        const Vector3& cameraPosition = camera->getPosition();
        pCB1->loadData(&cameraPosition, FLOAT3);
        int lightCount = light_manager->getShadowLights().size();
        pCB1->loadData(&lightCount, INT);

        const Vector3 cameraView = camera->getTransform().forward();
        pCB1->loadData(&cameraView, FLOAT3);
        pCB1->loadData(nullptr, FLOAT);

        const std::vector<ShadowLight*> shadow_lights =
            light_manager->getShadowLights();

        // Sun Cascade Data
        const SunLight* sun_light = light_manager->getSunLight();

        const Vector3 sun_direction = sun_light->getDirection();
        pCB1->loadData(&sun_direction, FLOAT3);
        pCB1->loadData(nullptr, FLOAT);

        for (int i = 0; i < SUN_NUM_CASCADES; i++) {
            ShadowLight* light = shadow_lights[i];

            Vector3 position = light->getPosition();
            pCB1->loadData(&position, FLOAT3);

            pCB1->loadData(nullptr, FLOAT);

            const Color& color = light->getColor();
            pCB1->loadData(&color, FLOAT3);
            pCB1->loadData(nullptr, INT);

            const Matrix4 m_world_to_local = light->getWorldMatrix().inverse();
            pCB1->loadData(&m_world_to_local, FLOAT4X4);

            const Matrix4& m_local_to_frustum = light->getFrustumMatrix();
            pCB1->loadData(&m_local_to_frustum, FLOAT4X4);

            const NormalizedShadowViewport normalized_view =
                light_manager->normalizeViewport(light->getShadowmapViewport());
            pCB1->loadData(&normalized_view, FLOAT4);
        }

        // ShadowLight Data
        for (int i = SUN_NUM_CASCADES; i < shadow_lights.size(); i++) {
            ShadowLight* light = shadow_lights[i];

            Vector3 position = light->getPosition();
            pCB1->loadData(&position, FLOAT3);

            pCB1->loadData(nullptr, FLOAT);

            const Color& color = light->getColor();
            pCB1->loadData(&color, FLOAT3);

            pCB1->loadData(nullptr, INT);

            const Matrix4 m_world_to_local = light->getWorldMatrix().inverse();
            pCB1->loadData(&m_world_to_local, FLOAT4X4);

            const Matrix4& m_local_to_frustum = light->getFrustumMatrix();
            pCB1->loadData(&m_local_to_frustum, FLOAT4X4);

            const NormalizedShadowViewport normalized_view =
                light_manager->normalizeViewport(light->getShadowmapViewport());
            pCB1->loadData(&normalized_view, FLOAT4);
        }

        pipeline_manager->bindPixelCB(CB1);
    }

    // Load my Textures
    {
        // Texture* tex = resource_manager->getTexture("TerrainGrass");
        // context->PSSetShaderResources(0, 1, &tex->shader_view);

        const Texture* shadow_texture = light_manager->getAtlasTexture();
        context->PSSetShaderResources(1, 1, &shadow_texture->shader_view);
    }

    // Load my Samplers
    {
        ID3D11SamplerState* mesh_texture_sampler =
            resource_manager->getMeshSampler();
        context->PSSetSamplers(0, 1, &mesh_texture_sampler);

        ID3D11SamplerState* shadowmap_sampler =
            resource_manager->getShadowMapSampler();
        context->PSSetSamplers(1, 1, &shadowmap_sampler);
    }

    // TEMP
    /* const std::vector<Mesh*>& terrain_meshes = terrain->getTerrainMeshes();
     for (Mesh* mesh : terrain_meshes) {
         if (mesh == nullptr)
             continue;*/

    BufferPool* bpool = terrain->getMesh();
    if (bpool->getNumTriangles() > 0) {
        context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

        UINT buffer_stride = sizeof(float) * 3;
        UINT buffer_offset = 0;

        ID3D11Buffer* p_buffer = bpool->getPositionBuffer();
        context->IASetVertexBuffers(POSITION, 1, &p_buffer, &buffer_stride,
                                    &buffer_offset);
        ID3D11Buffer* n_buffer = bpool->getNormalBuffer();
        context->IASetVertexBuffers(NORMAL, 1, &n_buffer, &buffer_stride,
                                    &buffer_offset);

        ID3D11Buffer* i_buffer = bpool->getIndexBuffer();
        context->IASetIndexBuffer(i_buffer, DXGI_FORMAT_R32_UINT, 0);

        UINT numIndices = bpool->getNumTriangles() * 3;
        context->DrawIndexed(numIndices, 0, 0);
    }
#if defined(_DEBUG)
    gpu_timer.endTimer("Terrain Pass");
    CPUTimer::EndCPUTimer("Terrain Pass");
#endif
}

void VisualSystem::performRenderPass() {
#if defined(_DEBUG)
    gpu_timer.beginTimer("Render Pass");
    CPUTimer::BeginCPUTimer("Render Pass");
#endif

    pipeline_manager->bindPixelShader("TexturedMesh");
    bindActiveRenderTarget(EnableDepthStencil_TestAndWrite);

    // Vertex Constant Buffer 1:
    // Stores the camera view and projection matrices
    {
        CBHandle* vCB1 = pipeline_manager->getVertexCB(CB1);
        vCB1->clearData();

        const Matrix4 viewMatrix = camera->getWorldToCameraMatrix();
        vCB1->loadData(&viewMatrix, FLOAT4X4);

        const Matrix4 projectionMatrix = camera->getFrustumMatrix();
        vCB1->loadData(&projectionMatrix, FLOAT4X4);

        pipeline_manager->bindVertexCB(CB1);
    }

    // Pixel Constant Buffer 1: Light Data
    // Stores data that is needed for lighting / shadowing.
    {
        CBHandle* pCB1 = pipeline_manager->getPixelCB(CB1);
        pCB1->clearData();

        const Vector3& cameraPosition = camera->getPosition();
        pCB1->loadData(&cameraPosition, FLOAT3);
        int lightCount = light_manager->getShadowLights().size();
        pCB1->loadData(&lightCount, INT);

        const Vector3 cameraView = camera->getTransform().forward();
        pCB1->loadData(&cameraView, FLOAT3);
        pCB1->loadData(nullptr, FLOAT);

        const std::vector<ShadowLight*> shadow_lights =
            light_manager->getShadowLights();

        // Sun Cascade Data
        const SunLight* sun_light = light_manager->getSunLight();

        const Vector3 sun_direction = sun_light->getDirection();
        pCB1->loadData(&sun_direction, FLOAT3);
        pCB1->loadData(nullptr, FLOAT);

        for (int i = 0; i < SUN_NUM_CASCADES; i++) {
            ShadowLight* light = shadow_lights[i];

            Vector3 position = light->getPosition();
            pCB1->loadData(&position, FLOAT3);

            pCB1->loadData(nullptr, FLOAT);

            const Color& color = light->getColor();
            pCB1->loadData(&color, FLOAT3);
            pCB1->loadData(nullptr, INT);

            const Matrix4 m_world_to_local = light->getWorldMatrix().inverse();
            pCB1->loadData(&m_world_to_local, FLOAT4X4);

            const Matrix4& m_local_to_frustum = light->getFrustumMatrix();
            pCB1->loadData(&m_local_to_frustum, FLOAT4X4);

            const NormalizedShadowViewport normalized_view =
                light_manager->normalizeViewport(light->getShadowmapViewport());
            pCB1->loadData(&normalized_view, FLOAT4);
        }

        // ShadowLight Data
        for (int i = SUN_NUM_CASCADES; i < shadow_lights.size(); i++) {
            ShadowLight* light = shadow_lights[i];

            Vector3 position = light->getPosition();
            pCB1->loadData(&position, FLOAT3);

            pCB1->loadData(nullptr, FLOAT);

            const Color& color = light->getColor();
            pCB1->loadData(&color, FLOAT3);

            pCB1->loadData(nullptr, INT);

            const Matrix4 m_world_to_local = light->getWorldMatrix().inverse();
            pCB1->loadData(&m_world_to_local, FLOAT4X4);

            const Matrix4& m_local_to_frustum = light->getFrustumMatrix();
            pCB1->loadData(&m_local_to_frustum, FLOAT4X4);

            const NormalizedShadowViewport normalized_view =
                light_manager->normalizeViewport(light->getShadowmapViewport());
            pCB1->loadData(&normalized_view, FLOAT4);
        }

        pipeline_manager->bindPixelCB(CB1);
    }

    // Load my Textures
    {
        const Texture* shadow_texture = light_manager->getAtlasTexture();
        context->PSSetShaderResources(1, 1, &shadow_texture->shader_view);
    }

    // Load my Samplers
    {
        ID3D11SamplerState* mesh_texture_sampler =
            resource_manager->getMeshSampler();
        context->PSSetSamplers(0, 1, &mesh_texture_sampler);

        ID3D11SamplerState* shadowmap_sampler =
            resource_manager->getShadowMapSampler();
        context->PSSetSamplers(1, 1, &shadowmap_sampler);
    }

    const Texture* color_tex = resource_manager->getColorAtlas();
    context->PSSetShaderResources(0, 1, &color_tex->shader_view);

    // Testing for animations
    static float time = 0.0f;
    time += 0.01f;
    time = time - (int)time;

    for (const RenderableAsset& renderable_asset : renderable_meshes) {
        const Asset* asset = renderable_asset.asset;
        const std::vector<Mesh*>& meshes = asset->getMeshes();

        if (asset->isSkinned()) {
            pipeline_manager->bindVertexShader("SkinnedMesh");
            asset->applyAnimationAtTime(1, time);
        } else
            pipeline_manager->bindVertexShader("TexturedMesh");

        for (const Mesh* mesh : meshes) {

            const Material mat = mesh->material;

            // Pixel CB0: Mesh Material Data
            {
                CBHandle* pCB0 = pipeline_manager->getPixelCB(CB0);
                pCB0->clearData();

                const TextureRegion& region = mat.tex_region;
                pCB0->loadData(&region.x, FLOAT);
                pCB0->loadData(&region.y, FLOAT);
                pCB0->loadData(&region.width, FLOAT);
                pCB0->loadData(&region.height, FLOAT);

                pipeline_manager->bindPixelCB(CB0);
            }

            // Vertex CB2: Transform matrices
            const Matrix4& mLocalToWorld = renderable_asset.m_localToWorld;
            {
                CBHandle* vCB2 = pipeline_manager->getVertexCB(CB2);
                vCB2->clearData();
                // Load mesh vertex transformation matrix
                vCB2->loadData(&mLocalToWorld, FLOAT4X4);
                // Load mesh normal transformation matrix
                Matrix4 normalTransform = mLocalToWorld.inverse().transpose();
                vCB2->loadData(&(normalTransform), FLOAT4X4);
                pipeline_manager->bindVertexCB(CB2);
            }

            // Skinning
            if (asset->isSkinned()) {
                // Vertex CB3: Joint Matrices
                {
                    CBHandle* vCB3 = pipeline_manager->getVertexCB(CB3);
                    vCB3->clearData();

                    const std::vector<SkinJoint>& skin = asset->getSkinJoints();
                    for (int i = 0; i < skin.size(); i++) {
                        // SUPER INEFFICIENT RN
                        const Matrix4 skin_matrix =
                            skin[i].getTransform(skin[i].node) *
                            skin[i].m_inverse_bind;
                        const Matrix4 skin_normal_matrix =
                            skin_matrix.inverse().transpose();

                        vCB3->loadData(&skin_matrix, FLOAT4X4);
                        vCB3->loadData(&skin_normal_matrix, FLOAT4X4);
                    }

                    pipeline_manager->bindVertexCB(CB3);
                }
            }

            // Load each mesh
            context->IASetPrimitiveTopology(
                D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

            ID3D11Buffer* buffer;
            UINT stride, offset;

            stride = sizeof(float) * 3;
            offset = 0;

            buffer = mesh->vertex_streams[POSITION];
            context->IASetVertexBuffers(POSITION, 1, &buffer, &stride, &offset);

            stride = sizeof(float) * 2;
            offset = 0;

            buffer = mesh->vertex_streams[TEXTURE];
            context->IASetVertexBuffers(TEXTURE, 1, &buffer, &stride, &offset);

            stride = sizeof(float) * 3;
            offset = 0;

            buffer = mesh->vertex_streams[NORMAL];
            context->IASetVertexBuffers(NORMAL, 1, &buffer, &stride, &offset);

            if (asset->isSkinned()) {
                stride = sizeof(float) * 4;
                offset = 0;

                buffer = mesh->vertex_streams[JOINTS];
                context->IASetVertexBuffers(JOINTS, 1, &buffer, &stride,
                                            &offset);

                stride = sizeof(float) * 4;
                offset = 0;

                buffer = mesh->vertex_streams[WEIGHTS];
                context->IASetVertexBuffers(WEIGHTS, 1, &buffer, &stride,
                                            &offset);
            }

            context->IASetIndexBuffer(mesh->index_buffer, DXGI_FORMAT_R32_UINT,
                                      0);

            UINT numIndices = mesh->triangle_count * 3;
            context->DrawIndexed(numIndices, 0, 0);
        }
    }

#if defined(_DEBUG)
    gpu_timer.endTimer("Render Pass");
    CPUTimer::EndCPUTimer("Render Pass");
#endif
}

void VisualSystem::performLightFrustumPass() {
    pipeline_manager->bindVertexShader("LightFrustum");
    pipeline_manager->bindPixelShader("LightFrustum");

    const std::vector<ShadowLight*>& lights = light_manager->getShadowLights();

    if (lights.size() == 0)
        return;

    // Disable depth writes
    bindActiveRenderTarget(DisableDepthStencil);

    {
        CBHandle* vcb0 = pipeline_manager->getVertexCB(CB0);
        vcb0->clearData();

        // This matrix scales the unit cube to the frustum cube. The frustum
        // cube has x in [-1,1], y in [-1,1], z in [0,1].
        const Matrix4 m_camera_to_screen =
            camera->getFrustumMatrix() * camera->getWorldToCameraMatrix();
        vcb0->loadData(&m_camera_to_screen, FLOAT4X4);

        const Matrix4 transform = Matrix4::T_Translate(Vector3(0, 0, 0.5f)) *
                                  Matrix4::T_Scale(2, 2, 1);
        for (int i = 0; i < lights.size(); i++) {
            const Frustum frust = lights[i]->frustum();
            const Matrix4 m_frustum =
                frust.getFrustumToWorldMatrix() * transform;
            vcb0->loadData(&m_frustum, FLOAT4X4);

            /*if (i == 3) {
                VisualDebug::DrawFrustum(frust.getFrustumToWorldMatrix(),
                                         Color::Green());
            }*/
        }

        pipeline_manager->bindVertexCB(CB0);
    }

    {
        CBHandle* pCB0 = pipeline_manager->getPixelCB(CB0);
        pCB0->clearData();

        const Vector3& pos = camera->getPosition();
        pCB0->loadData(&pos, FLOAT3);

        pipeline_manager->bindPixelCB(CB0);
    }

    Asset* frustum_cube = resource_manager->getAsset("Cube");
    const Mesh* mesh = frustum_cube->getMesh(0);

    ID3D11Buffer* indexBuffer = mesh->index_buffer;
    ID3D11Buffer* vertexBuffer = mesh->vertex_streams[POSITION];
    int numIndices = mesh->triangle_count * 3;

    UINT vertexStride = sizeof(float) * 3;
    UINT vertexOffset = 0;

    context->OMSetBlendState(blend_state, NULL, 0xffffffff);

    context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    context->IASetVertexBuffers(POSITION, 1, &vertexBuffer, &vertexStride,
                                &vertexOffset);
    context->IASetIndexBuffer(indexBuffer, DXGI_FORMAT_R32_UINT, 0);

    context->DrawIndexedInstanced(numIndices, lights.size(), 0, 0, 1);
}

void VisualSystem::performWaterSurfacePass() {
    pipeline_manager->bindVertexShader("WaterSurface");
    pipeline_manager->bindPixelShader("WaterSurface");

    // Disable depth writes
    swapActiveRenderTarget();
    bindActiveRenderTarget(EnableDepthStencil_TestAndWrite);

    {
        CBHandle* vcb0 = pipeline_manager->getVertexCB(CB0);
        vcb0->clearData();

        const Matrix4 m_camera_to_screen =
            camera->getFrustumMatrix() * camera->getWorldToCameraMatrix();
        vcb0->loadData(&m_camera_to_screen, FLOAT4X4);

        const Vector3& camera_pos = camera->getPosition();
        vcb0->loadData(&camera_pos, FLOAT3);

        const float surface_height = 100.f;
        vcb0->loadData(&surface_height, FLOAT);

        pipeline_manager->bindVertexCB(CB0);
    }

    // VCB1: Wave Information
    const std::vector<WaveConfig>& wave_config =
        terrain->getWaterSurface()->getWaveConfig();
    const int num_waves = terrain->getWaterSurface()->getNumWaves();
    time_elapsed += 1 / 60.f;
    {
        CBHandle* vcb1 = pipeline_manager->getVertexCB(CB1);
        vcb1->clearData();

        vcb1->loadData(&time_elapsed, FLOAT);
        vcb1->loadData(&num_waves, INT);
        vcb1->loadData(nullptr, FLOAT2);

        for (int i = 0; i < num_waves; i++) {
            vcb1->loadData(&wave_config[i].dimension, INT);
            vcb1->loadData(&wave_config[i].period, FLOAT);
            vcb1->loadData(&wave_config[i].amplitude, FLOAT);
            vcb1->loadData(nullptr, FLOAT);
        }

        pipeline_manager->bindVertexCB(CB1);
    }

    {
        CBHandle* pcb0 = pipeline_manager->getPixelCB(CB0);
        pcb0->clearData();

        const Vector3& pos = camera->getPosition();
        pcb0->loadData(&pos, FLOAT3);
        pcb0->loadData(nullptr, FLOAT);

        pipeline_manager->bindPixelCB(CB0);
    }

    const Mesh* surface_mesh = terrain->getWaterSurface()->getSurfaceMesh();

    context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    ID3D11Buffer* indexBuffer = surface_mesh->index_buffer;
    context->IASetIndexBuffer(indexBuffer, DXGI_FORMAT_R32_UINT, 0);

    ID3D11Buffer* vertexBuffer = surface_mesh->vertex_streams[POSITION];
    UINT vertexStride = sizeof(float) * 3;
    UINT vertexOffset = 0;
    context->IASetVertexBuffers(POSITION, 1, &vertexBuffer, &vertexStride,
                                &vertexOffset);

    int numIndices = surface_mesh->triangle_count * 3;
    context->OMSetBlendState(blend_state, NULL, 0xffffffff);
    context->DrawIndexed(numIndices, 0, 0);
}

void VisualSystem::processUnderwater() {
#if defined(_DEBUG)
    gpu_timer.beginTimer("Underwater Pass");
#endif

    pipeline_manager->bindVertexShader("PostProcess");
    pipeline_manager->bindPixelShader("Underwater");

    // Set render target
    swapActiveRenderTarget();
    bindActiveRenderTarget(DisableDepthStencil);

    // Set samplers and texture resources
    ID3D11SamplerState* mesh_texture_sampler =
        resource_manager->getMeshSampler();
    context->PSSetSamplers(0, 1, &mesh_texture_sampler);
    context->PSSetShaderResources(0, 1, &render_target_src->shader_view);
    context->PSSetShaderResources(1, 1, &depth_stencil->shader_view);

    // Set resolution information
    {
        CBHandle* pCB0 = pipeline_manager->getPixelCB(CB0);
        pCB0->clearData();

        const float f_width = (float)screen_target->width;
        pCB0->loadData(&f_width, FLOAT);
        const float f_height = (float)screen_target->height;
        pCB0->loadData(&f_height, FLOAT);

        const float z_near = camera->getZNear();
        pCB0->loadData(&z_near, FLOAT);
        const float z_far = camera->getZFar();
        pCB0->loadData(&z_far, FLOAT);

        pipeline_manager->bindPixelCB(CB0);
    }

    // Set parameters
    {
        CBHandle* pCB1 = pipeline_manager->getPixelCB(CB1);
        pCB1->clearData();

        const Matrix4 m_project_to_world =
            (camera->getFrustumMatrix() * camera->getWorldToCameraMatrix())
                .inverse();
        pCB1->loadData(&m_project_to_world, FLOAT4X4);

        const Vector3& camera_pos = camera->getPosition();
        pCB1->loadData(&camera_pos, FLOAT3);

        // The lower the value, the slower it gets darker as you go deeper
        const float intensity_drop = 0.001f;
        pCB1->loadData(&intensity_drop, FLOAT);

        // DO NOT MODIFY. Precomputed values for the fog interpolation.
        const float d = camera->getZFar() * 0.5f;
        const Vector3 fog_params = Vector3(1.f / (d * d), -2.f / d, 1.f);
        pCB1->loadData(&fog_params, FLOAT3);

        // Water Visibility
        const float visibility = 1.5f;
        pCB1->loadData(&visibility, FLOAT);

        // Where the surface is. Brightest at the surface.
        const float surface_height = 100.f;
        pCB1->loadData(&surface_height, FLOAT);

        const Vector3 shallow_waters = Vector3(24.f, 154.f, 180.f) / 255.f;
        pCB1->loadData(&shallow_waters, FLOAT3);
        const Vector3 deep_waters = Vector3(5.f, 68.f, 94.f) / 255.f;
        pCB1->loadData(&deep_waters, FLOAT3);

        pipeline_manager->bindPixelCB(CB1);
    }

    // Bind and draw full screen quad
    UINT vertexStride = sizeof(float) * 4;
    UINT vertexOffset = 0;

    context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    context->IASetVertexBuffers(0, 1, &postprocess_quad, &vertexStride,
                                &vertexOffset);

    context->Draw(6, 0);

#if defined(_DEBUG)
    gpu_timer.endTimer("Underwater Pass");
#endif
}

void VisualSystem::renderFinish() {
    // We will render from our most reecntly used render target to the screen
    pipeline_manager->bindVertexShader("PostProcess");
    pipeline_manager->bindPixelShader("PostProcess");

    // Set render target
    context->OMSetRenderTargets(1, &screen_target->target_view, nullptr);
    context->RSSetViewports(1, &viewport);

    // Set samplers and texture resources
    ID3D11SamplerState* mesh_texture_sampler =
        resource_manager->getMeshSampler();
    context->PSSetSamplers(0, 1, &mesh_texture_sampler);
    context->PSSetShaderResources(0, 1, &render_target_dest->shader_view);

    // Set resolution information
    {
        CBHandle* pCB0 = pipeline_manager->getPixelCB(CB0);
        pCB0->clearData();

        const float f_width = (float)screen_target->width;
        pCB0->loadData(&f_width, FLOAT);
        const float f_height = (float)screen_target->height;
        pCB0->loadData(&f_height, FLOAT);

        const float z_near = camera->getZNear();
        pCB0->loadData(&z_near, FLOAT);
        const float z_far = camera->getZFar();
        pCB0->loadData(&z_far, FLOAT);

        pipeline_manager->bindPixelCB(CB0);
    }

    // Bind and draw full screen quad
    UINT vertexStride = sizeof(float) * 4;
    UINT vertexOffset = 0;

    context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    context->IASetVertexBuffers(0, 1, &postprocess_quad, &vertexStride,
                                &vertexOffset);

    context->Draw(6, 0);

    // Finally, present what we rendered to
    swap_chain->Present(1, 0);

    renderable_meshes.clear();
}

// --- Rendering Helper Methods ---
// BindActiveRenderTarget:
// Binds the currently active render target. There is an option
// to bind the depth stencil too, if needed.
void VisualSystem::bindActiveRenderTarget(RenderTargetBindFlags bind_flags) {
    ID3D11DepthStencilView* depth_view = nullptr;

    if (bind_flags == EnableDepthStencil_TestAndWrite ||
        bind_flags == EnableDepthStencil_TestNoWrite) {
        depth_view = depth_stencil->depth_view;

        ID3D11DepthStencilState* state = nullptr;
        if (bind_flags == EnableDepthStencil_TestAndWrite)
            state = resource_manager->DSState_TestAndWrite();
        else if (bind_flags == EnableDepthStencil_TestNoWrite)
            state = resource_manager->DSState_TestNoWrite();
        context->OMSetDepthStencilState(state, 0);
    }

    context->OMSetRenderTargets(1, &render_target_dest->target_view,
                                depth_view);
    context->RSSetViewports(1, &viewport);
}

void VisualSystem::swapActiveRenderTarget() {
    Texture* temp = render_target_dest;
    render_target_dest = render_target_src;
    render_target_src = temp;
}

#if defined(ENABLE_DEBUG_DRAWING)
void VisualSystem::renderDebugPoints() {
    std::vector<PointData>& points = VisualDebug::points;

    if (points.size() == 0)
        return;

    pipeline_manager->bindVertexShader("DebugPoint");
    pipeline_manager->bindPixelShader("DebugPoint");

    Asset* cube = resource_manager->getAsset("Cube");
    const Mesh* mesh = cube->getMesh(0);

    ID3D11Buffer* indexBuffer = mesh->index_buffer;
    ID3D11Buffer* vertexBuffer = mesh->vertex_streams[POSITION];
    int numIndices = mesh->triangle_count * 3;

    UINT vertexStride = sizeof(float) * 3;
    UINT vertexOffset = 0;

    context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    context->IASetVertexBuffers(POSITION, 1, &vertexBuffer, &vertexStride,
                                &vertexOffset);
    context->IASetIndexBuffer(indexBuffer, DXGI_FORMAT_R32_UINT, 0);

    const int numPoints = points.size();

    // Load data into the constant buffer handle, while removing points
    // which are expired
    CBHandle* vCB0 = pipeline_manager->getVertexCB(CB0);
    CBHandle* vCB1 = pipeline_manager->getVertexCB(CB1);

    vCB0->clearData();
    vCB1->clearData();

    for (int i = 0; i < points.size(); i++) {
        PointData& data = points[i];
        vCB0->loadData(&data.position, FLOAT3);
        vCB0->loadData(&data.scale, FLOAT);
        vCB0->loadData(&data.color, FLOAT3);
        vCB0->loadData(nullptr, FLOAT);
    }
    pipeline_manager->bindVertexCB(CB0);

    points.clear();

    if (numPoints > 0) {
        // Vertex Constant Buffer 1:
        // Stores the camera view and projection matrices
        vCB1->clearData();
        {
            const Matrix4 viewMatrix = camera->getWorldToCameraMatrix();
            vCB1->loadData(&viewMatrix, FLOAT4X4);

            const Matrix4 projectionMatrix = camera->getFrustumMatrix();
            vCB1->loadData(&projectionMatrix, FLOAT4X4);
        }
        pipeline_manager->bindVertexCB(CB1);

        context->DrawIndexedInstanced(numIndices, numPoints, 0, 0, 1);
    }
}

void VisualSystem::renderDebugLines() {
    std::vector<LinePoint>& lines = VisualDebug::lines;

    if (lines.size() == 0)
        return;

    pipeline_manager->bindVertexShader("DebugLine");
    CBHandle* vCB1 = pipeline_manager->getVertexCB(CB1);

    pipeline_manager->bindPixelShader("DebugLine");

    vCB1->clearData();

    // Load line data into a vertex buffer
    if (line_vbuffer != nullptr)
        line_vbuffer->Release();

    D3D11_BUFFER_DESC buff_desc = {};
    buff_desc.ByteWidth = sizeof(LinePoint) * lines.size();
    buff_desc.Usage = D3D11_USAGE_DEFAULT;
    buff_desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;

    D3D11_SUBRESOURCE_DATA sr_data = {0};
    sr_data.pSysMem = (void*)lines.data();

    device->CreateBuffer(&buff_desc, &sr_data, &line_vbuffer);

    UINT vertexStride = sizeof(LinePoint);
    UINT vertexOffset = 0;

    context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINELIST);
    context->IASetVertexBuffers(DEBUG_LINE, 1, &line_vbuffer, &vertexStride,
                                &vertexOffset);

    int numLines = lines.size() * 2;

    if (numLines > 0) {
        // Vertex Constant Buffer 1:
        // Stores the camera view and projection matrices
        vCB1->clearData();
        {
            const Matrix4 viewMatrix = camera->getWorldToCameraMatrix();
            vCB1->loadData(&viewMatrix, FLOAT4X4);

            const Matrix4 projectionMatrix = camera->getFrustumMatrix();
            vCB1->loadData(&projectionMatrix, FLOAT4X4);
        }
        pipeline_manager->bindVertexCB(CB1);

        context->Draw(numLines, 0);
    }
}
#endif

#if defined(_DEBUG)
// ImGui Initialize:
// Initializes the ImGui menu and associated data.
void VisualSystem::imGuiInitialize(HWND window) {
    // Initialize ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |=
        ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls
    io.ConfigFlags |=
        ImGuiConfigFlags_NavEnableGamepad; // Enable Gamepad Controls

    ImGui_ImplWin32_Init(window);
    ImGui_ImplDX11_Init(device, context);

    // Create GPU + CPU Timers
    gpu_timer.initialize(device, context);

    gpu_timer.createTimer("GPU Frametime");
    gpu_timer.createTimer("Shadow Pass");
    gpu_timer.createTimer("Terrain Pass");
    gpu_timer.createTimer("Render Pass");

    CPUTimer::Initialize();
    CPUTimer::CreateCPUTimer("CPU Frametime");
    CPUTimer::CreateCPUTimer("Render Prepare");
    CPUTimer::CreateCPUTimer("Shadow Pass");
    CPUTimer::CreateCPUTimer("Terrain Pass");
    CPUTimer::CreateCPUTimer("Render Pass");
}

// ImGuiPrepare:
// Creates a new frame for the ImGui system and begin tracking GPU time
// for the current frame
void VisualSystem::imGuiPrepare() {
    // Start the Dear ImGui frame
    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    // Begin timestamping
    gpu_timer.beginFrame();

    // ImGui::ShowDemoWindow(); // Show demo window! :)
}

// ImGuiFinish:
// Finish and present the ImGui window
void VisualSystem::imGuiFinish() {
    // Finish and Display GPU + CPU Times
    gpu_timer.endFrame();

    if (ImGui::CollapsingHeader("Rendering")) {
        ImGui::SeparatorText("CPU Times:");
        CPUTimer::DisplayCPUTimes();

        ImGui::SeparatorText("GPU Times:");
        gpu_timer.displayTimes();

        ImGui::SeparatorText("Shadow Atlas:");
        light_manager->getAtlasTexture()->displayImGui(512);
    }

    // Finish the ImGui Frame
    ImGui::Render();
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
}

// ImGuiShutDown:
// Shut down the ImGui system
void VisualSystem::imGuiShutdown() {
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
}

#endif

} // namespace Graphics
} // namespace Engine