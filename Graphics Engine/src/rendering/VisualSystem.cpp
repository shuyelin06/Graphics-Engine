#include "VisualSystem.h"

#include <assert.h>
#include <d3d11_1.h>

#include "VisualDebug.h"
#include "core/Frustum.h"
#include "datamodel/Object.h"

#include "math/Vector4.h"
#include "resources/BumpMapBuilder.h"

namespace Engine {

namespace Graphics {
struct VisualParameters {
    Vector3 sun_direction;
    Vector3 sun_color;

    // Underwater Parameters
    float intensity_drop;
    float visibility;

    VisualParameters() {
        sun_direction = Vector3(-3.0f, -1.0f, 0.0f);
        sun_direction.inplaceNormalize();
        sun_color = Vector3(1.f, 1.f, 0.0f);

        intensity_drop = 0.001f;
        visibility = 0.5f;
    }
};

struct VisualCache {
    float time;

    Matrix4 m_world_to_screen;
    Matrix4 m_screen_to_world;

    Vector4 resolution_info;
};

// Constructor
// Initializes the VisualSystem
VisualSystem::VisualSystem(HWND window) {
    config = new VisualParameters();
    cache = new VisualCache();

    bump_tex = nullptr;

    device = NULL;
    context = NULL;

    resource_manager = NULL;
    pipeline = NULL;

    swap_chain = NULL;
    screen_target = NULL;

    camera = nullptr;
    terrain = nullptr;

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
    depth_stencil_copy = new Texture(width, height);

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
    result =
        device->CreateTexture2D(&tex_desc, NULL, &depth_stencil_copy->texture);
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
        result = device->CreateShaderResourceView(
            depth_stencil_copy->texture, &resource_view_desc,
            &depth_stencil_copy->shader_view);
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

// InitializeManagers:
// Initializes different managers that the visual system uses.
void VisualSystem::initializeManagers() {
    HRESULT result;

    resource_manager = new ResourceManager(device, context);
    resource_manager->initializeResources();

    pipeline = new PipelineManager(device, context);

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
    ICPUTimer cpu_timer = CPUTimer::TrackCPUTime("CPU Frametime");
#endif

    // TEMP
    light_manager->updateTimeOfDay(15.f);

    // Clear the the screen color
    render_target_dest->clearAsRenderTarget(context, Color(0.f, 0.f, 0.f));

    // Upload CB0
    {
        IConstantBuffer pcb0_common = pipeline->loadPixelCB(CB0);

        const Vector3 cam_pos = camera->getPosition();
        const Vector3 cam_direc = camera->getTransform().forward();
        const float z_near = camera->getZNear();
        const float z_far = camera->getZFar();

        pcb0_common.loadData(&cam_pos, FLOAT3);
        pcb0_common.loadData(&z_near, FLOAT);
        pcb0_common.loadData(&cam_direc, FLOAT3);
        pcb0_common.loadData(&z_far, FLOAT);

        pcb0_common.loadData(&cache->m_screen_to_world, FLOAT4X4);
        pcb0_common.loadData(&cache->m_world_to_screen, FLOAT4X4);

        pcb0_common.loadData(&cache->resolution_info.x, FLOAT);
        pcb0_common.loadData(&cache->resolution_info.y, FLOAT);
    }

    // Prepare the shadow maps
    performShadowPass(); //..

    // Bind my atlases
    const Texture* color_atlas = resource_manager->getColorAtlas();
    context->PSSetShaderResources(0, 1, &color_atlas->shader_view);
    const Texture* shadow_atlas = light_manager->getAtlasTexture();
    context->PSSetShaderResources(1, 1, &shadow_atlas->shader_view);

    // Render terrain
    performTerrainPass();
    // Render meshes
    performRenderPass();

    // Rendering is different depending on if we're below
    // or above the surface of the water
    const Vector3& cam_pos = camera->getPosition();
    if (cam_pos.y <= terrain->getSurfaceLevel() + 3.5f) {
        performLightFrustumPass();
        // Underwater rendering
        processUnderwater();
    } else {
        // Above water rendering
        performWaterSurfacePass();
        processSky();
    }

#if defined(ENABLE_DEBUG_DRAWING)
    // Debug Functionality
    renderDebugPoints();
    renderDebugLines();
    VisualDebug::Clear();
#endif

    processDither();

#if defined(_DEBUG)
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
    ICPUTimer cpu_timer = CPUTimer::TrackCPUTime("Render Prepare");
#endif

#if defined(_DEBUG)
    imGuiConfig();
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

        if (asset->isSkinned())
            asset->applyAnimationAtTime(1, cache->time);

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

    // ---
    const int size = 750;
    BumpMapBuilder bump_builder = BumpMapBuilder(size, size);

    static float freq = 0.02f;
    ImGui::SliderFloat("Frequency", &freq, 0.001f, 0.5f);
    static float AMP = 2.f;
    ImGui::SliderFloat("Amplitude", &AMP, 1.f, 25.f);

    if (ImGui::Button("Regenerate") || bump_tex == nullptr) {
        bump_builder.samplePerlinNoise(120512, freq, AMP);

        if (bump_tex != nullptr) {
            bump_builder.update(bump_tex, context);
        } else
            bump_tex = bump_builder.generate(device, true);
    }

    bump_tex->displayImGui();

    // ---

    // TODO: Terrain is not included in shadows

    // Cluster shadows
    light_manager->clusterShadowCasters();

    // Update the values stored in my cache
    cache->time += 1 / 60.f;
    cache->m_world_to_screen =
        camera->getFrustumMatrix() * camera->getWorldToCameraMatrix();
    cache->m_screen_to_world = cache->m_world_to_screen.inverse();
    cache->resolution_info =
        Vector4((float)screen_target->width, (float)screen_target->height,
                camera->getZNear(), camera->getZFar());
}

// PerformShadowPass:
// Render the scene from each light's point of view, to populate
// its shadow map.
void VisualSystem::performShadowPass() {
#if defined(_DEBUG)
    IGPUTimer gpu_timer = GPUTimer::TrackGPUTime("Shadow Pass");
#endif

    pipeline->bindVertexShader("ShadowMap");
    pipeline->bindPixelShader("ShadowMap");

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
            IConstantBuffer vCB0 = pipeline->loadVertexCB(CB2);

            const Matrix4& m_world_to_local = light->getWorldMatrix().inverse();
            vCB0.loadData(&m_world_to_local, FLOAT4X4);
            const Matrix4 m_local_to_frustum = light->getFrustumMatrix();
            vCB0.loadData(&m_local_to_frustum, FLOAT4X4);
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
                IConstantBuffer vCB1 = pipeline->loadVertexCB(CB1);
                vCB1.loadData(&mLocalToWorld, FLOAT4X4);
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
}

void VisualSystem::performTerrainPass() {
#if defined(_DEBUG)
    IGPUTimer gpu_timer = GPUTimer::TrackGPUTime("Terrain Pass");
#endif

    pipeline->bindVertexShader("Terrain");
    pipeline->bindPixelShader("Terrain");

    // TEST
    bindActiveRenderTarget(EnableDepthStencil_TestAndWrite);
    context->ClearDepthStencilView(depth_stencil->depth_view, D3D11_CLEAR_DEPTH,
                                   1.0f, 0);

    // Vertex Constant Buffer 0:
    // Stores the camera view and projection matrices
    {
        IConstantBuffer vCB0 = pipeline->loadVertexCB(CB0);
        vCB0.loadData(&cache->m_world_to_screen, FLOAT4X4);
    }

    // Pixel Constant Buffer 1: Light Data
    // Stores data that is needed for lighting / shadowing.
    {
        IConstantBuffer pCB1 = pipeline->loadPixelCB(CB1);
        light_manager->bindLightData(pCB1);
    }

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
}

void VisualSystem::performRenderPass() {
#if defined(_DEBUG)
    IGPUTimer gpu_timer = GPUTimer::TrackGPUTime("Render Pass");
#endif

    pipeline->bindPixelShader("TexturedMesh");
    bindActiveRenderTarget(EnableDepthStencil_TestAndWrite);

    // Vertex Constant Buffer 1:
    // Stores the camera view and projection matrices
    {
        IConstantBuffer vCB1 = pipeline->loadVertexCB(CB1);

        const Matrix4 viewMatrix = camera->getWorldToCameraMatrix();
        vCB1.loadData(&viewMatrix, FLOAT4X4);
        const Matrix4 projectionMatrix = camera->getFrustumMatrix();
        vCB1.loadData(&projectionMatrix, FLOAT4X4);
    }

    // Pixel Constant Buffer 1: Light Data
    // Stores data that is needed for lighting / shadowing.
    {
        IConstantBuffer pCB1 = pipeline->loadPixelCB(CB1);
        light_manager->bindLightData(pCB1);
    }

    // Testing for animations
    for (const RenderableAsset& renderable_asset : renderable_meshes) {
        const Asset* asset = renderable_asset.asset;
        const std::vector<Mesh*>& meshes = asset->getMeshes();

        if (asset->isSkinned()) {
            pipeline->bindVertexShader("SkinnedMesh");
        } else
            pipeline->bindVertexShader("TexturedMesh");

        for (const Mesh* mesh : meshes) {

            const Material mat = mesh->material;

            // Pixel CB2: Mesh Material Data
            {
                IConstantBuffer pCB2 = pipeline->loadPixelCB(CB2);

                const TextureRegion& region = mat.tex_region;
                pCB2.loadData(&region.x, FLOAT);
                pCB2.loadData(&region.y, FLOAT);
                pCB2.loadData(&region.width, FLOAT);
                pCB2.loadData(&region.height, FLOAT);
            }

            // Vertex CB2: Transform matrices
            const Matrix4& mLocalToWorld = renderable_asset.m_localToWorld;
            {
                IConstantBuffer vCB2 = pipeline->loadVertexCB(CB2);

                // Load mesh vertex transformation matrix
                vCB2.loadData(&mLocalToWorld, FLOAT4X4);
                // Load mesh normal transformation matrix
                Matrix4 normalTransform = mLocalToWorld.inverse().transpose();
                vCB2.loadData(&(normalTransform), FLOAT4X4);
            }

            // Skinning
            if (asset->isSkinned()) {
                // Vertex CB3: Joint Matrices
                {
                    IConstantBuffer vCB3 = pipeline->loadVertexCB(CB3);

                    const std::vector<SkinJoint>& skin = asset->getSkinJoints();
                    for (int i = 0; i < skin.size(); i++) {
                        // SUPER INEFFICIENT RN
                        const Matrix4 skin_matrix =
                            skin[i].getTransform(skin[i].node) *
                            skin[i].m_inverse_bind;
                        const Matrix4 skin_normal_matrix =
                            skin_matrix.inverse().transpose();

                        vCB3.loadData(&skin_matrix, FLOAT4X4);
                        vCB3.loadData(&skin_normal_matrix, FLOAT4X4);
                    }
                }
            }

            // Draw each mesh
            pipeline->drawMesh(mesh, 0, INDEX_LIST_END, 1);
        }
    }
}

void VisualSystem::performLightFrustumPass() {
#if defined(_DEBUG)
    IGPUTimer gpu_timer = GPUTimer::TrackGPUTime("Light Frustum Pass");
#endif

    pipeline->bindVertexShader("LightFrustum");
    pipeline->bindPixelShader("LightFrustum");

    const std::vector<ShadowLight*>& lights = light_manager->getShadowLights();

    if (lights.size() <= SUN_NUM_CASCADES)
        return;

    const int num_lights = lights.size() - SUN_NUM_CASCADES;

    // Disable depth writes
    bindActiveRenderTarget(EnableDepthStencil_TestNoWrite);

    {
        IConstantBuffer vcb0 = pipeline->loadVertexCB(CB0);

        // This matrix scales the unit cube to the frustum cube. The frustum
        // cube has x in [-1,1], y in [-1,1], z in [0,1].
        const Matrix4 m_camera_to_screen =
            camera->getFrustumMatrix() * camera->getWorldToCameraMatrix();
        vcb0.loadData(&m_camera_to_screen, FLOAT4X4);

        const Matrix4 transform = Matrix4::T_Translate(Vector3(0, 0, 0.5f)) *
                                  Matrix4::T_Scale(2, 2, 1);
        for (int i = 0; i < num_lights; i++) {
            const Frustum frust = lights[i + SUN_NUM_CASCADES]->frustum();
            const Matrix4 m_frustum =
                frust.getFrustumToWorldMatrix() * transform;
            vcb0.loadData(&m_frustum, FLOAT4X4);

            /*if (i == 3) {
                VisualDebug::DrawFrustum(frust.getFrustumToWorldMatrix(),
                                         Color::Green());
            }*/
        }
    }

    context->OMSetBlendState(blend_state, NULL, 0xffffffff);

    Asset* frustum_cube = resource_manager->getAsset("Cube");
    const Mesh* cube_mesh = frustum_cube->getMesh(0);
    pipeline->drawMesh(cube_mesh, INDEX_LIST_START, INDEX_LIST_END, num_lights);
}

void VisualSystem::performWaterSurfacePass() {
#if defined(_DEBUG)
    IGPUTimer gpu_timer = GPUTimer::TrackGPUTime("Water Surface Pass");
#endif

    pipeline->bindVertexShader("WaterSurface");
    pipeline->bindPixelShader("WaterSurface");

    // Copy depth stencil over
    context->CopyResource(depth_stencil_copy->texture, depth_stencil->texture);
    // Disable depth writes
    // swapActiveRenderTarget();

    bindActiveRenderTarget(EnableDepthStencil_TestAndWrite);

    {
        IConstantBuffer vcb0 = pipeline->loadVertexCB(CB0);
        const Matrix4 m_camera_to_screen =
            camera->getFrustumMatrix() * camera->getWorldToCameraMatrix();
        vcb0.loadData(&m_camera_to_screen, FLOAT4X4);

        const Vector3& camera_pos = camera->getPosition();
        vcb0.loadData(&camera_pos, FLOAT3);

        const float surface_height = terrain->getSurfaceLevel();
        vcb0.loadData(&surface_height, FLOAT);
    }

    // VCB1: Wave Information
    const std::vector<WaveConfig>& wave_config =
        terrain->getWaterSurface()->getWaveConfig();
    const int num_waves = terrain->getWaterSurface()->getNumWaves();
    {
        IConstantBuffer vcb1 = pipeline->loadVertexCB(CB1);

        vcb1.loadData(&cache->time, FLOAT);
        vcb1.loadData(&num_waves, INT);
        vcb1.loadData(nullptr, FLOAT2);

        for (int i = 0; i < num_waves; i++) {
            vcb1.loadData(&wave_config[i].direction, FLOAT2);
            vcb1.loadData(&wave_config[i].period, FLOAT);
            vcb1.loadData(&wave_config[i].amplitude, FLOAT);
        }
    }

    {
        IConstantBuffer pcb2 = pipeline->loadPixelCB(CB2);

        pcb2.loadData(&config->sun_direction, FLOAT3);
        pcb2.loadData(nullptr, FLOAT);

        pcb2.loadData(&config->sun_color, FLOAT3);
        pcb2.loadData(nullptr, FLOAT);
    }

    context->PSSetShaderResources(2, 1, &depth_stencil_copy->shader_view);
    context->PSSetShaderResources(3, 1, &bump_tex->shader_view);

    const float STARTING_SCALE = 5.f;
    constexpr int NUM_LODS = 3;
    {
        IConstantBuffer vCB2 = pipeline->loadVertexCB(CB2);

        float scale = STARTING_SCALE;
        for (int i = 0; i < NUM_LODS; i++) {
            Vector2 m_scale;

            m_scale = Vector2(scale, 0);
            vCB2.loadData(&m_scale, FLOAT2);
            vCB2.loadData(nullptr, FLOAT2);
            m_scale = Vector2(0, scale);
            vCB2.loadData(&m_scale, FLOAT2);
            vCB2.loadData(nullptr, FLOAT2);

            m_scale = Vector2(0, scale);
            vCB2.loadData(&m_scale, FLOAT2);
            vCB2.loadData(nullptr, FLOAT2);
            m_scale = Vector2(-scale, 0);
            vCB2.loadData(&m_scale, FLOAT2);
            vCB2.loadData(nullptr, FLOAT2);

            m_scale = Vector2(-scale, 0);
            vCB2.loadData(&m_scale, FLOAT2);
            vCB2.loadData(nullptr, FLOAT2);
            m_scale = Vector2(0, -scale);
            vCB2.loadData(&m_scale, FLOAT2);
            vCB2.loadData(nullptr, FLOAT2);

            m_scale = Vector2(0, -scale);
            vCB2.loadData(&m_scale, FLOAT2);
            vCB2.loadData(nullptr, FLOAT2);
            m_scale = Vector2(scale, 0);
            vCB2.loadData(&m_scale, FLOAT2);
            vCB2.loadData(nullptr, FLOAT2);

            scale *= 2;
        }
    }

    context->OMSetBlendState(blend_state, NULL, 0xffffffff);

    const Mesh* surface_mesh = terrain->getWaterSurface()->getSurfaceMesh();
    const int total_triangles = surface_mesh->triangle_count;
    const int num_inner_tri =
        terrain->getWaterSurface()->getNumInnerTriangles();

    pipeline->drawMesh(surface_mesh, 0, num_inner_tri, 4);
    pipeline->drawMesh(surface_mesh, num_inner_tri, total_triangles,
                       NUM_LODS * 4);
}

void VisualSystem::processSky() {
#if defined(_DEBUG)
    IGPUTimer gpu_timer = GPUTimer::TrackGPUTime("Sky Processing");
#endif

    pipeline->bindVertexShader("PostProcess");
    pipeline->bindPixelShader("Sky");

    swapActiveRenderTarget();
    bindActiveRenderTarget(DisableDepthStencil);

    // Set samplers and texture resources
    context->PSSetShaderResources(2, 1, &render_target_src->shader_view);
    context->PSSetShaderResources(3, 1, &depth_stencil->shader_view);

    {
        IConstantBuffer pcb2 = pipeline->loadPixelCB(CB2);

        pcb2.loadData(&config->sun_direction, FLOAT3);
        const float sun_size = 0.025f;
        pcb2.loadData(&sun_size, FLOAT);

        pcb2.loadData(&config->sun_color, FLOAT3);
        pcb2.loadData(nullptr, FLOAT);
        const Vector3 sky_color =
            Vector3(173.f / 255.f, 216.f / 255.f, 230.f / 255.f);
        pcb2.loadData(&sky_color, FLOAT3);
        pcb2.loadData(nullptr, FLOAT);
    }

    pipeline->drawPostProcessQuad();
}

void VisualSystem::processUnderwater() {
#if defined(_DEBUG)
    IGPUTimer gpu_timer = GPUTimer::TrackGPUTime("Underwater Pass");
#endif

    pipeline->bindVertexShader("PostProcess");
    pipeline->bindPixelShader("Underwater");

    // Set render target
    swapActiveRenderTarget();
    bindActiveRenderTarget(DisableDepthStencil);

    // Set samplers and texture resources
    context->PSSetShaderResources(2, 1, &render_target_src->shader_view);
    context->PSSetShaderResources(3, 1, &depth_stencil->shader_view);

    // Set parameters
    {
        IConstantBuffer pCB2 = pipeline->loadPixelCB(CB2);

        // DO NOT MODIFY. Precomputed values for the fog interpolation.
        const float d = camera->getZFar() * 0.5f;
        const Vector3 fog_params = Vector3(1.f / (d * d), -2.f / d, 1.f);
        pCB2.loadData(&fog_params, FLOAT3);

        // Water Visibility
        pCB2.loadData(&config->visibility, FLOAT);

        // Where the surface is. Brightest at the surface.
        const float surface_height = terrain->getSurfaceLevel();
        pCB2.loadData(&surface_height, FLOAT);

        const Vector3 shallow_waters = Vector3(24.f, 154.f, 180.f) / 255.f;
        pCB2.loadData(&shallow_waters, FLOAT3);

        // The lower the value, the slower it gets darker as you go deeper
        pCB2.loadData(&config->intensity_drop, FLOAT);

        const Vector3 deep_waters = Vector3(5.f, 68.f, 94.f) / 255.f;
        pCB2.loadData(&deep_waters, FLOAT3);
    }

    // Bind and draw full screen quad
    pipeline->drawPostProcessQuad();
}

void VisualSystem::processDither() {
#if defined(_DEBUG)
    IGPUTimer gpu_timer = GPUTimer::TrackGPUTime("Finish + Dither");
#endif

    // We will render from our most reecntly used render target to the screen
    pipeline->bindVertexShader("PostProcess");
    pipeline->bindPixelShader("PostProcess");

    // Set render target
    context->OMSetRenderTargets(1, &screen_target->target_view, nullptr);
    context->RSSetViewports(1, &viewport);

    // Set samplers and texture resources
    context->PSSetShaderResources(0, 1, &render_target_dest->shader_view);

    pipeline->drawPostProcessQuad();
}

void VisualSystem::renderFinish() {
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

    pipeline->bindVertexShader("DebugPoint");
    pipeline->bindPixelShader("DebugPoint");

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
    {
        IConstantBuffer vCB0 = pipeline->loadVertexCB(CB2);

        for (int i = 0; i < points.size(); i++) {
            PointData& data = points[i];
            vCB0.loadData(&data.position, FLOAT3);
            vCB0.loadData(&data.scale, FLOAT);
            vCB0.loadData(&data.color, FLOAT3);
            vCB0.loadData(nullptr, FLOAT);
        }
    }

    points.clear();

    if (numPoints > 0) {
        // Vertex Constant Buffer 1:
        // Stores the camera view and projection matrices
        {
            IConstantBuffer vCB1 = pipeline->loadVertexCB(CB1);

            const Matrix4 viewMatrix = camera->getWorldToCameraMatrix();
            vCB1.loadData(&viewMatrix, FLOAT4X4);
            const Matrix4 projectionMatrix = camera->getFrustumMatrix();
            vCB1.loadData(&projectionMatrix, FLOAT4X4);
        }

        context->DrawIndexedInstanced(numIndices, numPoints, 0, 0, 1);
    }
}

void VisualSystem::renderDebugLines() {
    std::vector<LinePoint>& lines = VisualDebug::lines;

    if (lines.size() == 0)
        return;

    pipeline->bindVertexShader("DebugLine");
    pipeline->bindPixelShader("DebugLine");

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
        {
            IConstantBuffer vCB1 = pipeline->loadVertexCB(CB1);
            const Matrix4 viewMatrix = camera->getWorldToCameraMatrix();
            vCB1.loadData(&viewMatrix, FLOAT4X4);
            const Matrix4 projectionMatrix = camera->getFrustumMatrix();
            vCB1.loadData(&projectionMatrix, FLOAT4X4);
        }

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
    GPUTimer::Initialize(device, context);
    CPUTimer::Initialize();
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
    GPUTimer::BeginFrame();

    // ImGui::ShowDemoWindow(); // Show demo window! :)
}

// ImGuiConfig:
// Sets configuration parameters
void VisualSystem::imGuiConfig() {
    static float sun_direc[3] = {-3.0f, -1.0f, 0.0f};
    static float sun_color[3] = {1.f, 1.f, 0.0f};

    if (ImGui::CollapsingHeader("Rendering Parameters")) {
        ImGui::SliderFloat3("Sun Direction", sun_direc, -5.f, 5.f);
        ImGui::SliderFloat3("Sun Color", sun_color, 0.0f, 1.f);

        ImGui::SeparatorText("Underwater Parameters");
        ImGui::SliderFloat("Intensity Drop", &config->intensity_drop, 0.00001f,
                           0.5f);
        ImGui::SliderFloat("Visibility", &config->visibility, 0.f, 1.f);
    }

    config->sun_direction =
        Vector3(sun_direc[0], sun_direc[1], sun_direc[2]).unit();
    config->sun_color = Vector3(sun_color[0], sun_color[1], sun_color[2]);
}

// ImGuiFinish:
// Finish and present the ImGui window
void VisualSystem::imGuiFinish() {
    // Finish and Display GPU + CPU Times
    GPUTimer::EndFrame();

    if (ImGui::CollapsingHeader("Rendering")) {
        ImGui::SeparatorText("CPU Times:");
        CPUTimer::DisplayCPUTimes();

        ImGui::SeparatorText("GPU Times:");
        GPUTimer::DisplayGPUTimes();

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