#include "VisualSystem.h"

#include <assert.h>
#include <d3d11_1.h>

#include "VisualDebug.h"

#include "datamodel/Object.h"

#define RGB(rgb) ((rgb) / 255.f)

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
    assetManager = new ResourceManager(device, context);
    assetManager->initialize();

    shaderManager = new ShaderManager(device);
    shaderManager->initialize();

    texture_manager = new TextureManager(device);
    
    TextureAtlas* shadow_atlas = new TextureAtlas(
        texture_manager->createShadowTexture("ShadowAtlas", 1024, 1024));
    light_manager = new LightManager(shadow_atlas);

    texture_manager->createDepthTexture("DepthStencilMain", width, height);

    // Create my global light (the sun).
    // It will always be at index 0 of the lights vector.
    createLight(QUALITY_1);
    createLight(QUALITY_0); // TEMP

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

// CreateLight:
// Creates and returns a light in the visual system
ShadowLight* VisualSystem::createLight() { return createLight(QUALITY_1); }
ShadowLight* VisualSystem::createLight(ShadowMapQuality quality) {
    return light_manager->createShadowLight(quality);
}

// DrawRequests:
// Methods that let other components of the application submit requests
// for rendering
void VisualSystem::drawAsset(const AssetRenderRequest& request) {
    assetRequests.push_back(request);
}

void VisualSystem::drawTerrain(const TerrainRenderRequest& request) {
    terrainRequests.push_back(request);
}

// Render:
// Renders the entire scene to the screen.
void VisualSystem::render() {
#if defined(_DEBUG)
    gpu_timer.beginTimer("GPU Frametime");
    cpu_timer.beginTimer("CPU Frametime");
#endif

    renderPrepare();

    performShadowPass();

    performTerrainPass();
    performRenderPass();

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

    // Parse all asset render requests, by querying the asset manager for the
    // assets.
    for (const AssetRenderRequest& request : assetRequests) {
        Asset* asset = assetManager->getAsset(request.slot);

        for (Mesh* mesh : asset->getMeshes()) {
            ShadowCaster shadowCaster;
            shadowCaster.mesh = mesh;
            shadowCaster.m_localToWorld = request.mLocalToWorld;

            shadow_casters.push_back(shadowCaster);
        }

        RenderableAsset renderableAsset;
        renderableAsset.asset = asset;
        renderableAsset.m_localToWorld = request.mLocalToWorld;
        renderable_assets.push_back(renderableAsset);
    }

    assetRequests.clear();

    // Parse all terrain render requests. This may involve generating the
    // terrain data.
    for (const TerrainRenderRequest& request : terrainRequests) {
        // Offset the terrain chunks so that they are adjacent to one another,
        // and are centered around the origin.
        const float x_offset =
            request.x_offset * TERRAIN_SIZE - CHUNK_X_LIMIT / 2 * TERRAIN_SIZE;
        const float z_offset =
            request.z_offset * TERRAIN_SIZE - CHUNK_Z_LIMIT / 2 * TERRAIN_SIZE;
        const float y_offset = -TERRAIN_HEIGHT * 0.75f;

        Mesh* mesh = assetManager->getTerrainMesh(
            request.x_offset, request.z_offset, request.data);

        ShadowCaster shadowCaster;
        shadowCaster.mesh = mesh;
        shadowCaster.m_localToWorld =
            Transform::GenerateTranslationMatrix(x_offset, y_offset, z_offset);
        shadow_casters.push_back(shadowCaster);

        RenderableTerrain renderableTerrain;
        renderableTerrain.mesh = mesh;
        renderableTerrain.terrain_offset =
            Vector3(x_offset, y_offset, z_offset);
        renderable_terrain.push_back(renderableTerrain);
    }

    terrainRequests.clear();

    // Set sun position to be
    ShadowLight* sun_light = light_manager->getShadowLight(0);

    sun_light->getTransform()->setViewDirection(Vector3(0, -0.25f, 0.75f));

    // Vector3 position = camera.getTransform()->getPosition() +
    // sun_light->getTransform()->backward() * 125; // 75 OG
    Vector3 position = sun_light->getTransform()->backward() * 75; // 75 OG
    sun_light->getTransform()->setPosition(position.x, position.y, position.z);

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
    const std::vector<ShadowLight*>& shadow_lights =
        light_manager->getShadowLights();
    for (ShadowLight* light : shadow_lights) {
        // Load light view and projection matrix
        vCB0->clearData();

        const Matrix4 viewMatrix = light->getWorldToLightMatrix();
        vCB0->loadData(&viewMatrix, FLOAT4X4);
        const Matrix4 projectionMatrix = light->getProjectionMatrix();
        vCB0->loadData(&projectionMatrix, FLOAT4X4);

        // Set the light as the render target.
        const ShadowMapViewport& shadow_viewport =
            light->getShadowmapViewport();

        D3D11_VIEWPORT viewport = {};
        viewport.TopLeftX = shadow_viewport.x;
        viewport.TopLeftY = shadow_viewport.y;
        viewport.Width = shadow_viewport.width;
        viewport.Height = shadow_viewport.height;
        viewport.MinDepth = 0.0f;
        viewport.MaxDepth = 1.0f;

        context->OMSetRenderTargets(0, nullptr, shadow_texture->depth_view);
        context->RSSetViewports(1, &viewport);

        // For each asset, load its mesh
        for (const ShadowCaster& caster : shadow_casters) {
            Mesh* mesh = caster.mesh;
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
    const Matrix4 projectionMatrix = camera.getProjectionMatrix();
    vCB0->loadData(&projectionMatrix, FLOAT4X4);

    // Pixel Constant Buffer 1:
    // Stores camera position, and the scene's light instances.
    pCB1->clearData();
    {
        const Vector3& cameraPosition = camera.getTransform()->getPosition();
        pCB1->loadData(&cameraPosition, FLOAT3);

        int lightCount = light_manager->getShadowLights().size();
        pCB1->loadData(&lightCount, INT);

        const std::vector<ShadowLight*>& shadow_lights =
            light_manager->getShadowLights();
        for (ShadowLight* light : shadow_lights) {
            const Vector3& position = light->getTransform()->getPosition();
            pCB1->loadData(&position, FLOAT3);

            pCB1->loadData(nullptr, FLOAT);

            const Color& color = light->getColor();
            pCB1->loadData(&color, FLOAT3);

            pCB1->loadData(nullptr, INT);

            const Matrix4 viewMatrix = light->getWorldToLightMatrix();
            pCB1->loadData(&viewMatrix, FLOAT4X4);

            const Matrix4 projectionMatrix = light->getProjectionMatrix();
            pCB1->loadData(&projectionMatrix, FLOAT4X4);

            const NormalizedShadowViewport normalized_view =
                light_manager->normalizeViewport(light->getShadowmapViewport());
            pCB1->loadData(&normalized_view, FLOAT4);
        }
    }

    // Load my Textures
    {
        Texture* tex = assetManager->getTexture(TerrainGrass);
        context->PSSetShaderResources(0, 1, &tex->shader_view);

        const Texture* shadow_texture = light_manager->getAtlasTexture();
        context->PSSetShaderResources(1, 1, &shadow_texture->shader_view);
    }

    // Load my Samplers
    {
        ID3D11SamplerState* mesh_texture_sampler =
            assetManager->getSampler(MeshTexture);
        context->PSSetSamplers(0, 1, &mesh_texture_sampler);

        ID3D11SamplerState* shadowmap_sampler =
            assetManager->getSampler(ShadowMap);
        context->PSSetSamplers(1, 1, &shadowmap_sampler);
    }

    for (const RenderableTerrain& terrain : renderable_terrain) {
        Mesh* mesh = terrain.mesh;
        const Vector3& offset = terrain.terrain_offset;

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
    context->ClearDepthStencilView(depth_texture->depth_view, D3D11_CLEAR_DEPTH,
                                   1.0f, 0);
    context->RSSetViewports(1, &viewport);

    // Vertex Constant Buffer 1:
    // Stores the camera view and projection matrices
    vCB1->clearData();
    {
        const Matrix4 viewMatrix = camera.getWorldToCameraMatrix();
        vCB1->loadData(&viewMatrix, FLOAT4X4);

        const Matrix4 projectionMatrix = camera.getProjectionMatrix();
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

        const std::vector<ShadowLight*>& shadow_lights =
            light_manager->getShadowLights();
        for (ShadowLight* light : shadow_lights) {
            const Vector3& position = light->getTransform()->getPosition();
            pCB1->loadData(&position, FLOAT3);

            pCB1->loadData(nullptr, FLOAT);

            const Color& color = light->getColor();
            pCB1->loadData(&color, FLOAT3);

            pCB1->loadData(nullptr, INT);

            const Matrix4 viewMatrix = light->getWorldToLightMatrix();
            pCB1->loadData(&viewMatrix, FLOAT4X4);

            const Matrix4 projectionMatrix = light->getProjectionMatrix();
            pCB1->loadData(&projectionMatrix, FLOAT4X4);

            const NormalizedShadowViewport normalized_view =
                light_manager->normalizeViewport(light->getShadowmapViewport());
            pCB1->loadData(&normalized_view, FLOAT4);

            // DEBUG
            const Matrix4 frustumMatrix =
                viewMatrix.inverse() * projectionMatrix.inverse();
            VisualDebug::DrawFrustum(frustumMatrix, Color::Green());
        }
    }

    // Load my Textures
    {
        Texture* tex = assetManager->getTexture(Perlin);
        context->PSSetShaderResources(0, 1, &tex->shader_view);

        const Texture* shadow_texture = light_manager->getAtlasTexture();
        context->PSSetShaderResources(1, 1, &shadow_texture->shader_view);
    }

    // Load my Samplers
    {
        ID3D11SamplerState* mesh_texture_sampler =
            assetManager->getSampler(MeshTexture);
        context->PSSetSamplers(0, 1, &mesh_texture_sampler);

        ID3D11SamplerState* shadowmap_sampler =
            assetManager->getSampler(ShadowMap);
        context->PSSetSamplers(1, 1, &shadowmap_sampler);
    }

    for (const RenderableAsset& command : renderable_assets) {
        Asset* asset = command.asset;
        const Matrix4& mLocalToWorld = command.m_localToWorld;

        {
            vCB2->clearData();
            // Load mesh vertex transformation matrix
            vCB2->loadData(&mLocalToWorld, FLOAT4X4);
            // Load mesh normal transformation matrix
            Matrix4 normalTransform = mLocalToWorld.inverse().transpose();
            vCB2->loadData(&(normalTransform), FLOAT4X4);
        }

        // Load each mesh
        for (Mesh* mesh : asset->getMeshes()) {
            context->IASetPrimitiveTopology(
                D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

            ID3D11Buffer* buffers[3] = {mesh->vertex_streams[POSITION],
                                        mesh->vertex_streams[TEXTURE],
                                        mesh->vertex_streams[NORMAL]};
            UINT strides[3] = {sizeof(float) * 3, sizeof(float) * 2,
                               sizeof(float) * 3};
            UINT offsets[3] = {0, 0, 0};

            context->IASetVertexBuffers(0, 3, buffers, strides, offsets);
            context->IASetIndexBuffer(mesh->index_buffer, DXGI_FORMAT_R32_UINT,
                                      0);

            vShader->bindShader(device, context);
            pShader->bindShader(device, context);

            UINT numIndices = mesh->triangle_count * 3;
            context->DrawIndexed(numIndices, 0, 0);
        }
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
    renderable_assets.clear();
}

void VisualSystem::renderDebugPoints() {
    VertexShader* vShader = shaderManager->getVertexShader("DebugPoint");
    CBHandle* vCB1 = vShader->getCBHandle(CB1);

    PixelShader* pShader = shaderManager->getPixelShader("DebugPoint");

    vShader->getCBHandle(CB0)->clearData();
    vShader->getCBHandle(CB1)->clearData();

    Asset* cube = assetManager->getAsset(Cube);
    Mesh* mesh = cube->getMesh(0);

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

    int numPoints = VisualDebug::LoadPointData(vShader->getCBHandle(CB0));

    if (numPoints > 0) {
        // Vertex Constant Buffer 1:
        // Stores the camera view and projection matrices
        vCB1->clearData();
        {
            const Matrix4 viewMatrix = camera.getWorldToCameraMatrix();
            vCB1->loadData(&viewMatrix, FLOAT4X4);

            const Matrix4 projectionMatrix = camera.getProjectionMatrix();
            vCB1->loadData(&projectionMatrix, FLOAT4X4);
        }

        vShader->bindShader(device, context);
        pShader->bindShader(device, context);

        context->DrawIndexedInstanced(numIndices, numPoints, 0, 0, 1);
    }
}

void VisualSystem::renderDebugLines() {
    VertexShader* vShader = shaderManager->getVertexShader("DebugLine");
    CBHandle* vCB1 = vShader->getCBHandle(CB1);
    
    PixelShader* pShader = shaderManager->getPixelShader("DebugLine");

    vShader->getCBHandle(CB1)->clearData();

    int numLines = VisualDebug::LoadLineData(context, device);

    if (numLines > 0) {
        // Vertex Constant Buffer 1:
        // Stores the camera view and projection matrices
        vCB1->clearData();
        {
            const Matrix4 viewMatrix = camera.getWorldToCameraMatrix();
            vCB1->loadData(&viewMatrix, FLOAT4X4);

            const Matrix4 projectionMatrix = camera.getProjectionMatrix();
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
        light_manager->getAtlasTexture()->displayImGui(256);
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