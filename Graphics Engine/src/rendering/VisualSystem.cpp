#include "VisualSystem.h"

#include <assert.h>
#include <d3d11_1.h>

#include "VisualDebug.h"

#include "datamodel/Object.h"

#include "core/Frustum.h"

#define RGB(rgb) ((rgb) / 255.f)

#include "datamodel/TreeGenerator.h"
namespace Engine {

namespace Graphics {
// Constructor
// Saves the handle to the application window and initializes the
// system's data structures
VisualSystem::VisualSystem() {
    camera = Camera();

    device = NULL;
    context = NULL;

    shader_manager = NULL;
    resource_manager = NULL;

    swap_chain = NULL;
    screen_target = NULL;
}

// GetCamera:
// Returns the camera
const Camera& VisualSystem::getCamera() const { return camera; }

Camera& VisualSystem::getCamera() { return camera; }

// Initialize:
// Initializes the Visual Engine by creating the necessary Direct3D11
// components.
void VisualSystem::initialize(HWND window) {
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
    result = D3D11CreateDeviceAndSwapChain(
        NULL, D3D_DRIVER_TYPE_HARDWARE, NULL,
        D3D11_CREATE_DEVICE_SINGLETHREADED, // Flags
        NULL, 0, D3D11_SDK_VERSION, &swap_chain_descriptor, &swap_chain,
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

    // Create my render target texture
    render_target = new Texture(width, height);

    D3D11_TEXTURE2D_DESC tex_desc = {};
    tex_desc.Width = width;
    tex_desc.Height = height;
    tex_desc.MipLevels = 1;
    tex_desc.ArraySize = 1;
    tex_desc.MipLevels = 1;
    tex_desc.ArraySize = 1;
    tex_desc.SampleDesc.Count = 1;
    tex_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    tex_desc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;

    result = device->CreateTexture2D(&tex_desc, NULL, &render_target->texture);
    assert(SUCCEEDED(result));

    // Create a target view for my render target texture so we can render to it
    result = device->CreateRenderTargetView(render_target->texture, 0,
                                            &render_target->target_view);
    assert(SUCCEEDED(result));

    // Create a shader view for my render target texture so we can access it in
    // the GPU
    {
        D3D11_SHADER_RESOURCE_VIEW_DESC resource_view_desc = {};
        resource_view_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        resource_view_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
        resource_view_desc.Texture2D.MostDetailedMip = 0;
        resource_view_desc.Texture2D.MipLevels = 1;

        result = device->CreateShaderResourceView(render_target->texture,
                                                  &resource_view_desc,
                                                  &render_target->shader_view);
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
    resource_manager->initialize();

    shader_manager = new ShaderManager(device);
    shader_manager->initialize();

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

// Shutdown:
// Closes the visual system.
void VisualSystem::shutdown() {
#if defined(_DEBUG)
    imGuiShutdown();
#endif
}

// Create Objects:
// Creates and returns objects in the visual system
AssetObject* VisualSystem::bindAssetObject(Object* object,
                                           const std::string& asset_name) {
    Asset* asset = resource_manager->getAsset(asset_name);
    AssetObject* asset_obj = new AssetObject(object, asset);
    object->setVisualObject(asset_obj);
    renderable_assets.push_back(asset_obj);
    return asset_obj;
}

ShadowLightObject* VisualSystem::bindShadowLightObject(Object* object) {
    ShadowLight* light = light_manager->createShadowLight(QUALITY_1);
    ShadowLightObject* light_obj = new ShadowLightObject(object, light);
    object->setVisualObject(light_obj);
    shadow_lights.push_back(light_obj);
    return light_obj;
}

VisualTerrain* VisualSystem::bindVisualTerrain(TerrainChunk* terrain) {
    // TEMP: Generate a triangulation of the terrain
    MeshBuilder* builder = resource_manager->createMeshBuilder();
    VisualTerrain* visual_terrain = new VisualTerrain(terrain, builder);

    terrain_chunks.push_back(visual_terrain);
    terrain->bindVisualTerrain(visual_terrain);

    return visual_terrain;
}

// Render:
// Renders the entire scene to the screen.
void VisualSystem::render() {
#if defined(_DEBUG)
    gpu_timer.beginTimer("GPU Frametime");
    cpu_timer.beginTimer("CPU Frametime");
#endif

    renderPrepare();

    performShadowPass(); //..

    performTerrainPass();
    performRenderPass();

    processSky();

    // ...
#if defined(_DEBUG)
    // Debug Functionality
    renderDebugPoints();
    renderDebugLines();
    VisualDebug::Clear();
#endif

#if defined(_DEBUG)
    gpu_timer.endTimer("GPU Frametime");
    cpu_timer.endTimer("CPU Frametime");
    imGuiFinish();
#endif

    renderFinish();

#if defined(_DEBUG)
    // Prepare for the next frame
    imGuiPrepare();
#endif
}

int generateTreeMeshHelper(MeshBuilder& builder,
                           const std::vector<TreeStructure>& grammar, int index,
                           const Vector3& position, const Vector2& rotation) {
    if (index >= grammar.size())
        return -1;

    const TreeStructure tree = grammar[index];

    switch (tree.token) {
    case TRUNK: {
        const float phi = rotation.u;
        const float theta = rotation.v;

        Vector3 direction = SphericalToEuler(1.0, theta, phi);
        const Quaternion rotation_offset =
            Quaternion::RotationAroundAxis(Vector3::PositiveX(), -PI / 2);
        direction = rotation_offset.rotationMatrix3() * direction;

        const Vector3 next_pos =
            position + direction * tree.trunk_data.trunk_length;

        builder.setColor(Color(150.f / 255.f, 75.f / 255.f, 0));
        builder.addTube(position, next_pos, tree.trunk_data.trunk_thickess, 5);
        return generateTreeMeshHelper(builder, grammar, index + 1, next_pos,
                                      rotation);
    } break;

    case BRANCH: {
        const Vector2 new_rotation =
            rotation + Vector2(tree.branch_data.branch_angle_phi,
                               tree.branch_data.branch_angle_theta);

        const int next_index = generateTreeMeshHelper(
            builder, grammar, index + 1, position, new_rotation);
        return generateTreeMeshHelper(builder, grammar, next_index, position,
                                      rotation);
    } break;

    case LEAF: {
        builder.setColor(Color::Green());

        const Vector3 random_axis =
            Vector3(1 + Random(0.f, 1.f), Random(0.f, 1.f), Random(0.f, 1.f))
                .unit();
        const float angle = Random(0, 2 * PI);

        builder.addCube(position,
                        Quaternion::RotationAroundAxis(random_axis, angle),
                        tree.leaf_data.leaf_density);
        return index + 1;
    } break;
    }

    return -1;
}

void generateTreeMesh(MeshBuilder& builder,
                      const std::vector<TreeStructure>& grammar,
                      const Vector3& offset) {
    // Rotation stores (phi, theta), spherical angles. rho is assumed to be 1.
    generateTreeMeshHelper(builder, grammar, 0, offset, Vector2(0, 0));
}

// RenderPrepare:
// Prepares the engine for rendering, by processing all render requests and
// clearing the screen
void VisualSystem::renderPrepare() {
#if defined(_DEBUG)
    cpu_timer.beginTimer("Render Prepare");
#endif

    // Clear the the screen color
    render_target->clearAsRenderTarget(
        context, Color(RGB(158.f), RGB(218.f), RGB(255.f)));

    // Check and remove any visual objects that are no longer valid
    int head;

    head = 0;
    for (int i = 0; i < renderable_assets.size(); i++) {
        if (!renderable_assets[i]->markedForDestruction()) {
            renderable_assets[head] = renderable_assets[i];
            head++;
        } else {
            delete renderable_assets[i];
            renderable_assets[i] = nullptr;
        }
    }
    renderable_assets.resize(head);

    // Check and remove any visual terrain objects that are no longer valid
    head = 0;
    for (int i = 0; i < terrain_chunks.size(); i++) {
        if (!terrain_chunks[i]->markedForDestruction()) {
            terrain_chunks[head] = terrain_chunks[i];
            head++;
        } else {
            delete terrain_chunks[i];
            terrain_chunks[i] = nullptr;
        }
    }
    terrain_chunks.resize(head);

    // --- TESTING ENVIRONMENT ---
    Asset* cube = resource_manager->getAsset("Capybara");
    cube->getMesh(0)->material.base_color_tex->displayImGui();

    // --- TEST ---
    static TreeGenerator tree_gen = TreeGenerator();
    static AssetObject* tree_asset = nullptr;

    if (ImGui::Button("Regenerate")) {
        tree_gen.generateTree();

        const std::vector<TreeStructure> tree = tree_gen.getTree();

        MeshBuilder* builder = resource_manager->createMeshBuilder();
        generateTreeMesh(*builder, tree, Vector3(0, 0, 0));
        builder->regenerateNormals();

        if (tree_asset == nullptr) {
            Object* obj = new Object();
            obj->getTransform().setPosition(0, 50.f, 0);
            obj->updateLocalMatrix(Matrix4::Identity());

            tree_asset = new AssetObject(obj, new Asset());
            tree_asset->asset->addMesh(builder->generate());
            obj->setVisualObject(tree_asset);
            renderable_assets.push_back(tree_asset);
        } else {
            tree_asset->asset = new Asset();
            tree_asset->asset->addMesh(builder->generate());
        }
    }

    // tree_gen.debugDrawTree(Vector3(0, 0, 0));
    // ---

    // --- TEST 2: Sun
    static float time = 15.f;
    ImGui::SliderFloat("Time of Day: ", &time, 1.0f, 23.f);
    light_manager->updateTimeOfDay(time);

    // Pull information from the datamodel
    // - Pull asset local -> world matrices from the datamodel
    // - Pull light data from the datamodel.
    for (AssetObject* asset_object : renderable_assets)
        asset_object->pullDatamodelData();
    for (ShadowLightObject* shadow_light : shadow_lights)
        shadow_light->pullDatamodelData();
    light_manager->updateSunCascades(camera.frustum());

    static bool enable = true;
    ImGui::Checkbox("Disable Frustum Culling", &enable);
    const Frustum cam_frustum = camera.frustum();

    // Prepare managers for data
    light_manager->resetShadowCasters();

    for (const AssetObject* object : renderable_assets) {
        const Asset* asset = object->getAsset();

        const std::vector<Mesh*>& meshes = asset->getMeshes();
        for (const Mesh* mesh : meshes) {
            ShadowCaster shadowCaster;
            shadowCaster.mesh = mesh;
            shadowCaster.m_localToWorld = object->object->getLocalMatrix();
            light_manager->addShadowCaster(shadowCaster);

            if (enable || cam_frustum.intersectsOBB(
                              OBB(mesh->aabb, Matrix4::Identity()))) {
                RenderableMesh renderableMesh;
                renderableMesh.mesh = mesh;
                renderableMesh.m_localToWorld =
                    object->object->getLocalMatrix();
                renderable_meshes.push_back(renderableMesh);
            }
        }
    }

    // Parse all terrain data
    for (const VisualTerrain* terrain : terrain_chunks) {
        ShadowCaster shadowCaster;
        shadowCaster.m_localToWorld = Matrix4::Identity();

        shadowCaster.mesh = terrain->terrain_mesh;
        light_manager->addShadowCaster(shadowCaster);

        for (Mesh* tree_mesh : terrain->tree_meshes) {
            shadowCaster.mesh = tree_mesh;
            light_manager->addShadowCaster(shadowCaster);

            if (enable || cam_frustum.intersectsOBB(
                              OBB(tree_mesh->aabb, Matrix4::Identity()))) {
                RenderableMesh renderableMesh;
                renderableMesh.mesh = tree_mesh;
                renderableMesh.m_localToWorld = Matrix4::Identity();
                renderable_meshes.push_back(renderableMesh);
            }
        }

        terrain_meshes.push_back(terrain->terrain_mesh);
    }

    // Cluster shadows
    light_manager->clusterShadowCasters();

#if defined(_DEBUG)
    cpu_timer.endTimer("Render Prepare");
#endif
}

// PerformShadowPass:
// Render the scene from each light's point of view, to populate
// its shadow map.
void VisualSystem::performShadowPass() {
#if defined(_DEBUG)
    gpu_timer.beginTimer("Shadow Pass");
    cpu_timer.beginTimer("Shadow Pass");
#endif

    VertexShader* vShader = shader_manager->getVertexShader("ShadowMap");
    CBHandle* vCB0 = vShader->getCBHandle(CB0);
    CBHandle* vCB1 = vShader->getCBHandle(CB1);

    PixelShader* pShader = shader_manager->getPixelShader("ShadowMap");

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
        vCB0->clearData();

        const Matrix4& m_world_to_local = light->getWorldMatrix().inverse();
        vCB0->loadData(&m_world_to_local, FLOAT4X4);
        const Matrix4 m_local_to_frustum = light->getFrustumMatrix();
        vCB0->loadData(&m_local_to_frustum, FLOAT4X4);

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

            vCB1->clearData();
            vCB1->loadData(&mLocalToWorld, FLOAT4X4);

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

            vShader->bindShader(device, context);
            pShader->bindShader(device, context);

            context->DrawIndexed(num_indices, 0, 0);
        }
    }

#if defined(_DEBUG)
    gpu_timer.endTimer("Shadow Pass");
    cpu_timer.endTimer("Shadow Pass");
#endif
}

void VisualSystem::performTerrainPass() {
#if defined(_DEBUG)
    gpu_timer.beginTimer("Terrain Pass");
    cpu_timer.beginTimer("Terrain Pass");
#endif

    VertexShader* vShader = shader_manager->getVertexShader("Terrain");
    CBHandle* vCB0 = vShader->getCBHandle(CB0);

    PixelShader* pShader = shader_manager->getPixelShader("Terrain");
    CBHandle* pCB0 = pShader->getCBHandle(CB0);
    CBHandle* pCB1 = pShader->getCBHandle(CB1);

    // TEST
    context->OMSetRenderTargets(1, &render_target->target_view,
                                depth_stencil->depth_view);
    context->ClearDepthStencilView(depth_stencil->depth_view, D3D11_CLEAR_DEPTH,
                                   1.0f, 0);
    context->RSSetViewports(1, &viewport);

    // Vertex Constant Buffer 0:
    // Stores the camera view and projection matrices
    vCB0->clearData();

    const Matrix4 viewMatrix = camera.getWorldToCameraMatrix();
    vCB0->loadData(&viewMatrix, FLOAT4X4);
    const Matrix4 projectionMatrix = camera.getFrustumMatrix();
    vCB0->loadData(&projectionMatrix, FLOAT4X4);

    // Pixel Constant Buffer 1: Light Data
    // Stores data that is needed for lighting / shadowing.
    pCB1->clearData();
    {
        const Vector3& cameraPosition = camera.getTransform()->getPosition();
        pCB1->loadData(&cameraPosition, FLOAT3);
        int lightCount = light_manager->getShadowLights().size();
        pCB1->loadData(&lightCount, INT);

        const Vector3 cameraView = camera.getTransform()->forward();
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
    for (Mesh* mesh : terrain_meshes) {
        context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

        UINT buffer_stride = sizeof(float) * 3;
        UINT buffer_offset = 0;

        context->IASetVertexBuffers(POSITION, 1,
                                    &mesh->vertex_streams[POSITION],
                                    &buffer_stride, &buffer_offset);
        context->IASetVertexBuffers(NORMAL, 1, &mesh->vertex_streams[NORMAL],
                                    &buffer_stride, &buffer_offset);

        context->IASetIndexBuffer(mesh->index_buffer, DXGI_FORMAT_R32_UINT, 0);

        vShader->bindShader(device, context);
        pShader->bindShader(device, context);

        UINT numIndices = mesh->triangle_count * 3;
        context->DrawIndexed(numIndices, 0, 0);
    }

#if defined(_DEBUG)
    gpu_timer.endTimer("Terrain Pass");
    cpu_timer.endTimer("Terrain Pass");
#endif
}

void VisualSystem::performRenderPass() {
#if defined(_DEBUG)
    gpu_timer.beginTimer("Render Pass");
    cpu_timer.beginTimer("Render Pass");
#endif

    VertexShader* vShader = shader_manager->getVertexShader("TexturedMesh");
    CBHandle* vCB1 = vShader->getCBHandle(CB1);
    CBHandle* vCB2 = vShader->getCBHandle(CB2);

    PixelShader* pShader = shader_manager->getPixelShader("TexturedMesh");
    CBHandle* pCB0 = pShader->getCBHandle(CB0);
    CBHandle* pCB1 = pShader->getCBHandle(CB1);

    context->OMSetRenderTargets(1, &render_target->target_view,
                                depth_stencil->depth_view);
    context->RSSetViewports(1, &viewport);

    // Vertex Constant Buffer 1:
    // Stores the camera view and projection matrices
    vCB1->clearData();
    {
        const Matrix4 viewMatrix = camera.getWorldToCameraMatrix();
        vCB1->loadData(&viewMatrix, FLOAT4X4);

        const Matrix4 projectionMatrix = camera.getFrustumMatrix();
        vCB1->loadData(&projectionMatrix, FLOAT4X4);
    }

    // Pixel Constant Buffer 1: Light Data
    // Stores data that is needed for lighting / shadowing.
    pCB1->clearData();
    {
        const Vector3& cameraPosition = camera.getTransform()->getPosition();
        pCB1->loadData(&cameraPosition, FLOAT3);
        int lightCount = light_manager->getShadowLights().size();
        pCB1->loadData(&lightCount, INT);

        const Vector3 cameraView = camera.getTransform()->forward();
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

    for (const RenderableMesh& renderable_mesh : renderable_meshes) {
        const Mesh* mesh = renderable_mesh.mesh;
        const Material mat = mesh->material;

        // FOR NOW: IGNORE MESHES WITHOHOUT BASE COLOR TEX
        if (mat.base_color_tex == nullptr)
            continue;

        Texture* tex = mat.base_color_tex;
        context->PSSetShaderResources(0, 1, &tex->shader_view);

        const Matrix4& mLocalToWorld = renderable_mesh.m_localToWorld;

        {
            vCB2->clearData();
            // Load mesh vertex transformation matrix
            vCB2->loadData(&mLocalToWorld, FLOAT4X4);
            // Load mesh normal transformation matrix
            Matrix4 normalTransform = mLocalToWorld.inverse().transpose();
            vCB2->loadData(&(normalTransform), FLOAT4X4);
        }

        // Load each mesh
        context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

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

        context->IASetIndexBuffer(mesh->index_buffer, DXGI_FORMAT_R32_UINT, 0);

        vShader->bindShader(device, context);
        pShader->bindShader(device, context);

        UINT numIndices = mesh->triangle_count * 3;
        context->DrawIndexed(numIndices, 0, 0);
    }

#if defined(_DEBUG)
    gpu_timer.endTimer("Render Pass");
    cpu_timer.endTimer("Render Pass");
#endif
}

void VisualSystem::processSky() {
    VertexShader* vShader = shader_manager->getVertexShader("PostProcess");

    PixelShader* pShader = shader_manager->getPixelShader("Sky");
    CBHandle* pCB0 = pShader->getCBHandle(CB0);
    CBHandle* pCB1 = pShader->getCBHandle(CB1);

    // Set render target
    context->OMSetRenderTargets(1, &screen_target->target_view, nullptr);
    context->RSSetViewports(1, &viewport);

    // Set Data
    ID3D11SamplerState* mesh_texture_sampler =
        resource_manager->getMeshSampler();
    context->PSSetSamplers(0, 1, &mesh_texture_sampler);

    context->PSSetShaderResources(0, 1, &render_target->shader_view);
    context->PSSetShaderResources(1, 1, &depth_stencil->shader_view);

    pCB0->clearData();
    {
        const float f_width = (float)screen_target->width;
        pCB0->loadData(&f_width, FLOAT);
        const float f_height = (float)screen_target->height;
        pCB0->loadData(&f_height, FLOAT);

        static int kernel_size = 1;
        ImGui::SliderInt("Kernel Size", &kernel_size, 1, 100);
        float f_kernel = (float)kernel_size;
        pCB0->loadData(&f_kernel, FLOAT);

        pCB0->loadData(nullptr, FLOAT);
    }

    pCB1->clearData();
    {
        const Matrix4 m_project_to_world =
            (camera.getFrustumMatrix() * camera.getWorldToCameraMatrix())
                .inverse();
        pCB1->loadData(&m_project_to_world, FLOAT4X4);

        const Vector3& sun_direction =
            light_manager->getSunLight()->getDirection();
        pCB1->loadData(&sun_direction, FLOAT3);

        pCB1->loadData(nullptr, FLOAT);

        const Vector3 cam_pos = camera.getTransform()->getPosition();
        pCB1->loadData(&cam_pos, FLOAT3);

        pCB1->loadData(nullptr, FLOAT);
    }

    // Bind full screen quad
    UINT vertexStride = sizeof(float) * 4;
    UINT vertexOffset = 0;

    context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    context->IASetVertexBuffers(0, 1, &postprocess_quad, &vertexStride,
                                &vertexOffset);

    vShader->bindShader(device, context);
    pShader->bindShader(device, context);

    context->Draw(6, 0);
}

void VisualSystem::renderFinish() {
    // Present what we rendered to
    swap_chain->Present(1, 0);

    renderable_meshes.clear();
    terrain_meshes.clear();
}

void VisualSystem::renderDebugPoints() {
    std::vector<PointData>& points = VisualDebug::points;

    if (points.size() == 0)
        return;

    VertexShader* vShader = shader_manager->getVertexShader("DebugPoint");
    CBHandle* vCB0 = vShader->getCBHandle(CB0);
    CBHandle* vCB1 = vShader->getCBHandle(CB1);

    PixelShader* pShader = shader_manager->getPixelShader("DebugPoint");

    vShader->getCBHandle(CB0)->clearData();
    vShader->getCBHandle(CB1)->clearData();

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

    vShader->bindShader(device, context);
    pShader->bindShader(device, context);

    const int numPoints = points.size();

    // Load data into the constant buffer handle, while removing points
    // which are expired
    for (int i = 0; i < points.size(); i++) {
        PointData& data = points[i];
        vCB0->loadData(&data.position, FLOAT3);
        vCB0->loadData(&data.scale, FLOAT);
        vCB0->loadData(&data.color, FLOAT3);
        vCB0->loadData(nullptr, FLOAT);
    }

    points.clear();

    if (numPoints > 0) {
        // Vertex Constant Buffer 1:
        // Stores the camera view and projection matrices
        vCB1->clearData();
        {
            const Matrix4 viewMatrix = camera.getWorldToCameraMatrix();
            vCB1->loadData(&viewMatrix, FLOAT4X4);

            const Matrix4 projectionMatrix = camera.getFrustumMatrix();
            vCB1->loadData(&projectionMatrix, FLOAT4X4);
        }

        vShader->bindShader(device, context);
        pShader->bindShader(device, context);

        context->DrawIndexedInstanced(numIndices, numPoints, 0, 0, 1);
    }
}

void VisualSystem::renderDebugLines() {
    std::vector<LinePoint>& lines = VisualDebug::lines;

    if (lines.size() == 0)
        return;

    VertexShader* vShader = shader_manager->getVertexShader("DebugLine");
    CBHandle* vCB1 = vShader->getCBHandle(CB1);

    PixelShader* pShader = shader_manager->getPixelShader("DebugLine");

    vShader->getCBHandle(CB1)->clearData();

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
            const Matrix4 viewMatrix = camera.getWorldToCameraMatrix();
            vCB1->loadData(&viewMatrix, FLOAT4X4);

            const Matrix4 projectionMatrix = camera.getFrustumMatrix();
            vCB1->loadData(&projectionMatrix, FLOAT4X4);
        }

        vShader->bindShader(device, context);
        pShader->bindShader(device, context);

        context->Draw(numLines, 0);
    }
}

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

    cpu_timer.initialize();

    cpu_timer.createTimer("CPU Frametime");
    cpu_timer.createTimer("Render Prepare");
    cpu_timer.createTimer("Shadow Pass");
    cpu_timer.createTimer("Terrain Pass");
    cpu_timer.createTimer("Render Pass");
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
        cpu_timer.displayTimes();

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