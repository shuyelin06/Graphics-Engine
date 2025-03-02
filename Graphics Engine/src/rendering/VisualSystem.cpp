#include "VisualSystem.h"

#include <assert.h>
#include <d3d11_1.h>

#include "VisualDebug.h"

#include "datamodel/Object.h"

#define RGB(rgb) ((rgb) / 255.f)

#include "datamodel/TreeGenerator.h"
namespace Engine {

namespace Graphics {
// Constructor
// Saves the handle to the application window and initializes the
// system's data structures
VisualSystem::VisualSystem(HWND _window) {
    window = _window;

    camera = Camera();

    device = NULL;
    context = NULL;

    shaderManager = NULL;
    assetManager = NULL;

    swap_chain = NULL;
    render_target_view = NULL;
}

// GetCamera:
// Returns the camera
const Camera& VisualSystem::getCamera() const { return camera; }

Camera& VisualSystem::getCamera() { return camera; }

// Initialize:
// Initializes the Visual Engine by creating the necessary Direct3D11
// components.
void VisualSystem::initialize() {
    HRESULT result;

    // Get window width and height
    RECT rect;

    GetClientRect(window, &rect);

    const UINT width = rect.right - rect.left;
    const UINT height = rect.bottom - rect.top;

    // Create swap chain, device, and context.
    // The swap chain is responsible for swapping between textures
    // for rendering.
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
    ID3D11Texture2D* framebuffer;

    result = swap_chain->GetBuffer(0, __uuidof(ID3D11Texture2D),
                                   (void**)&framebuffer);
    assert(SUCCEEDED(result));

    result =
        device->CreateRenderTargetView(framebuffer, 0, &render_target_view);
    assert(SUCCEEDED(result));

    framebuffer->Release(); // Free frame buffer (no longer needed)

    // Create my viewport
    viewport = {0.0f, 0.0f, (float)width, (float)height, 0.0f, 1.0f};

    // Create my managers
    assetManager = new VisualResourceManager(device, context);
    assetManager->initialize();

    shaderManager = new ShaderManager(device);
    shaderManager->initialize();

    texture_manager = new TextureManager(device);

    TextureAtlas* shadow_atlas = new TextureAtlas(
        texture_manager->createShadowTexture("ShadowAtlas", 2048, 2048));
    light_manager = new LightManager(shadow_atlas);

    texture_manager->createDepthTexture("DepthStencilMain", width, height);

#if defined(_DEBUG)
    imGuiInitialize();
    imGuiPrepare(); // Pre-Prepare a Frame
#endif
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
    Asset* asset = assetManager->getAsset(asset_name);
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
    MeshBuilder* builder = assetManager->createMeshBuilder();
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
// clearing the screen.
void VisualSystem::renderPrepare() {
#if defined(_DEBUG)
    cpu_timer.beginTimer("Render Prepare");
#endif

    // Clear the the screen color
    float color[4] = {RGB(158.f), RGB(218.f), RGB(255.f), 1.0f};
    context->ClearRenderTargetView(render_target_view, color);

    // --- TEST ---
    static TreeGenerator tree_gen = TreeGenerator();
    static AssetObject* tree_asset = nullptr;

    if (ImGui::Button("Regenerate")) {
        tree_gen.generateTree();

        const std::vector<TreeStructure> tree = tree_gen.getTree();

        MeshBuilder* builder = assetManager->createMeshBuilder();
        generateTreeMesh(*builder, tree, Vector3(0, 0, 0));
        builder->regenerateNormals();

        if (tree_asset == nullptr) {
            Object* obj = new Object();
            obj->getTransform().setPosition(0, 50.f, 0);
            obj->updateLocalMatrix(Matrix4::Identity());

            tree_asset = new AssetObject(obj, new Asset(builder->generate()));
            obj->setVisualObject(tree_asset);
            renderable_assets.push_back(tree_asset);
        } else {
            tree_asset->asset = new Asset(builder->generate());
        }
    }

    // tree_gen.debugDrawTree(Vector3(0, 0, 0));
    // ---

    // --- TEST 2
    static float direction[3] = {0, -0.5f, 0.25f};

    ImGui::SliderFloat3("Sun Direction", direction, -1.f, 1.f);
    light_manager->getSunLight()->setSunDirection(
        Vector3(direction[0], direction[1], direction[2]));

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

    // Update my light data from the datamodel.
    for (ShadowLightObject* shadow_light : shadow_lights)
        shadow_light->pullDatamodelData();
    light_manager->update(camera.getFrustum());

    for (const AssetObject* object : renderable_assets) {
        const Asset* asset = object->getAsset();

        ShadowCaster shadowCaster;
        shadowCaster.mesh = asset->getMesh();
        shadowCaster.m_localToWorld = object->object->getLocalMatrix();
        shadow_casters.push_back(shadowCaster);
    }

    // Parse all terrain data
    for (const VisualTerrain* terrain : terrain_chunks) {
        ShadowCaster shadowCaster;
        shadowCaster.mesh = terrain->terrain_mesh;
        shadowCaster.m_localToWorld = Matrix4::Identity();
        shadow_casters.push_back(shadowCaster);

        RenderableTerrain renderableTerrain;
        renderableTerrain.mesh = terrain->terrain_mesh;
        renderableTerrain.terrain_offset = Vector3();
        renderable_terrain.push_back(renderableTerrain);
    }

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

    VertexShader* vShader = shaderManager->getVertexShader("ShadowMap");
    CBHandle* vCB0 = vShader->getCBHandle(CB0);
    CBHandle* vCB1 = vShader->getCBHandle(CB1);

    PixelShader* pShader = shaderManager->getPixelShader("ShadowMap");

    const Texture* shadow_texture = light_manager->getAtlasTexture();
    context->ClearDepthStencilView(shadow_texture->depth_view,
                                   D3D11_CLEAR_DEPTH, 1.0f, 0);

    const std::vector<ShadowLight*> shadow_lights =
        light_manager->getShadowLights();
    for (ShadowLight* light : shadow_lights) {
        // Load light view and projection matrix
        vCB0->clearData();

        const Matrix4& m_world_to_local = light->getWorldMatrix().inverse();
        vCB0->loadData(&m_world_to_local, FLOAT4X4);

        const Matrix4 m_local_to_frustum = light->getFrustumMatrix();
        vCB0->loadData(&m_local_to_frustum, FLOAT4X4);

        VisualDebug::DrawFrustum(light->getWorldMatrix() *
                                     m_local_to_frustum.inverse(),
                                 Color::Green());

        // Set the light as the render target.
        const D3D11_VIEWPORT viewport = light->getShadowmapViewport().toD3D11();
        context->OMSetRenderTargets(0, nullptr, shadow_texture->depth_view);
        context->RSSetViewports(1, &viewport);

        // For each asset, load its mesh
        for (const ShadowCaster& caster : shadow_casters) {
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

    VertexShader* vShader = shaderManager->getVertexShader("Terrain");
    CBHandle* vCB0 = vShader->getCBHandle(CB0);
    CBHandle* vCB1 = vShader->getCBHandle(CB1);

    PixelShader* pShader = shaderManager->getPixelShader("Terrain");
    CBHandle* pCB0 = pShader->getCBHandle(CB0);
    CBHandle* pCB1 = pShader->getCBHandle(CB1);

    const Texture* depth_texture =
        texture_manager->getTexture("DepthStencilMain");

    context->OMSetRenderTargets(1, &render_target_view,
                                depth_texture->depth_view);
    context->ClearDepthStencilView(depth_texture->depth_view, D3D11_CLEAR_DEPTH,
                                   1.0f, 0);
    context->RSSetViewports(1, &viewport);

    // Vertex Constant Buffer 0:
    // Stores the camera view and projection matrices
    vCB0->clearData();

    const Matrix4 viewMatrix = camera.getWorldToCameraMatrix();
    vCB0->loadData(&viewMatrix, FLOAT4X4);
    const Matrix4 projectionMatrix = camera.getFrustumMatrix();
    vCB0->loadData(&projectionMatrix, FLOAT4X4);

    // Pixel Constant Buffer 1:
    // Stores camera position, and the scene's light instances.
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
        for (ShadowLight* light : shadow_lights) {
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
        Texture* tex = assetManager->getTexture("TerrainGrass");
        context->PSSetShaderResources(0, 1, &tex->shader_view);

        const Texture* shadow_texture = light_manager->getAtlasTexture();
        context->PSSetShaderResources(1, 1, &shadow_texture->shader_view);
    }

    // Load my Samplers
    {
        ID3D11SamplerState* mesh_texture_sampler =
            assetManager->getMeshSampler();
        context->PSSetSamplers(0, 1, &mesh_texture_sampler);

        ID3D11SamplerState* shadowmap_sampler =
            assetManager->getShadowMapSampler();
        context->PSSetSamplers(1, 1, &shadowmap_sampler);
    }

    // TEMP
    for (const RenderableTerrain terrain : renderable_terrain) {
        Mesh* mesh = terrain.mesh;
        const Vector3 offset = terrain.terrain_offset;

        vCB1->clearData();
        vCB1->loadData(&offset, FLOAT3);
        vCB1->loadData(nullptr, FLOAT);

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

    VertexShader* vShader = shaderManager->getVertexShader("ShadowShader");
    CBHandle* vCB1 = vShader->getCBHandle(CB1);
    CBHandle* vCB2 = vShader->getCBHandle(CB2);

    PixelShader* pShader = shaderManager->getPixelShader("ShadowShader");
    CBHandle* pCB0 = pShader->getCBHandle(CB0);
    CBHandle* pCB1 = pShader->getCBHandle(CB1);

    const Texture* depth_texture =
        texture_manager->getTexture("DepthStencilMain");

    context->OMSetRenderTargets(1, &render_target_view,
                                depth_texture->depth_view);
    /*context->ClearDepthStencilView(depth_texture->depth_view,
       D3D11_CLEAR_DEPTH, 1.0f, 0);*/
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

    // Pixel Constant Buffer 1:
    // Stores camera position, and the scene's light instances.
    pCB1->clearData();
    {
        const Vector3& cameraPosition = camera.getTransform()->getPosition();
        pCB1->loadData(&cameraPosition, FLOAT3);

        int lightCount = light_manager->getShadowLights().size();
        pCB1->loadData(&lightCount, INT);

        for (ShadowLightObject* light_obj : shadow_lights) {
            const Vector3& position =
                light_obj->object->getTransform().getPosition();
            pCB1->loadData(&position, FLOAT3);

            pCB1->loadData(nullptr, FLOAT);

            const Color& color = light_obj->light->getColor();
            pCB1->loadData(&color, FLOAT3);

            pCB1->loadData(nullptr, INT);

            const Matrix4 viewMatrix =
                light_obj->object->getLocalMatrix().inverse();
            pCB1->loadData(&viewMatrix, FLOAT4X4);

            const Matrix4 projectionMatrix =
                light_obj->light->getFrustumMatrix();
            pCB1->loadData(&projectionMatrix, FLOAT4X4);

            const NormalizedShadowViewport normalized_view =
                light_manager->normalizeViewport(
                    light_obj->light->getShadowmapViewport());
            pCB1->loadData(&normalized_view, FLOAT4);

            // DEBUG
            const Matrix4 frustumMatrix =
                viewMatrix.inverse() * projectionMatrix.inverse();
            // VisualDebug::DrawFrustum(frustumMatrix, Color::Green());
        }
    }

    // Load my Textures
    {
        Texture* tex = assetManager->getTexture("CapybaraTex");
        context->PSSetShaderResources(0, 1, &tex->shader_view);

        const Texture* shadow_texture = light_manager->getAtlasTexture();
        context->PSSetShaderResources(1, 1, &shadow_texture->shader_view);
    }

    // Load my Samplers
    {
        ID3D11SamplerState* mesh_texture_sampler =
            assetManager->getMeshSampler();
        context->PSSetSamplers(0, 1, &mesh_texture_sampler);

        ID3D11SamplerState* shadowmap_sampler =
            assetManager->getShadowMapSampler();
        context->PSSetSamplers(1, 1, &shadowmap_sampler);
    }

    for (const AssetObject* asset_obj : renderable_assets) {
        Asset* asset = asset_obj->asset;
        const Matrix4& mLocalToWorld = asset_obj->object->getLocalMatrix();

        {
            vCB2->clearData();
            // Load mesh vertex transformation matrix
            vCB2->loadData(&mLocalToWorld, FLOAT4X4);
            // Load mesh normal transformation matrix
            Matrix4 normalTransform = mLocalToWorld.inverse().transpose();
            vCB2->loadData(&(normalTransform), FLOAT4X4);
        }

        // Load each mesh
        const Mesh* mesh = asset->getMesh();

        context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

        ID3D11Buffer* buffer;
        UINT stride, offset;

        stride = sizeof(float) * 3;
        offset = 0;

        buffer = mesh->vertex_streams[POSITION];
        context->IASetVertexBuffers(POSITION, 1, &buffer, &stride, &offset);

        buffer = mesh->vertex_streams[NORMAL];
        context->IASetVertexBuffers(NORMAL, 1, &buffer, &stride, &offset);

        buffer = mesh->vertex_streams[COLOR];
        context->IASetVertexBuffers(COLOR, 1, &buffer, &stride, &offset);

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

void VisualSystem::renderFinish() {
    // Present what we rendered to
    swap_chain->Present(1, 0);

    shadow_casters.clear();
    renderable_terrain.clear();
}

void VisualSystem::renderDebugPoints() {
    std::vector<PointData>& points = VisualDebug::points;

    if (points.size() == 0)
        return;

    VertexShader* vShader = shaderManager->getVertexShader("DebugPoint");
    CBHandle* vCB0 = vShader->getCBHandle(CB0);
    CBHandle* vCB1 = vShader->getCBHandle(CB1);

    PixelShader* pShader = shaderManager->getPixelShader("DebugPoint");

    vShader->getCBHandle(CB0)->clearData();
    vShader->getCBHandle(CB1)->clearData();

    Asset* cube = assetManager->getAsset("Cube");
    const Mesh* mesh = cube->getMesh();

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

    VertexShader* vShader = shaderManager->getVertexShader("DebugLine");
    CBHandle* vCB1 = vShader->getCBHandle(CB1);

    PixelShader* pShader = shaderManager->getPixelShader("DebugLine");

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
void VisualSystem::imGuiInitialize() {
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