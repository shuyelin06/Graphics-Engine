#include "VisualSystem.h"

#include <assert.h>
#include <d3d11_1.h>

#include "VisualDebug.h"
#include "core/Frustum.h"
#include "datamodel/Object.h"
#include "datamodel/objects/DMCamera.h"

#include "math/Vector4.h"
#include "resources/BumpMapBuilder.h"

namespace Engine {

namespace Graphics {
struct VisualParameters {
    VisualParameters() = default;

    Vector3 sun_direction = Vector3(-3.0f, -1.0f, 0.0f).unit();
    Vector3 sun_color = Vector3(1.f, 1.f, 0.0f);

    // Underwater Parameters
    struct {
        float sky_multiplier = 0.35f;
        float scattering_multiplier = 0.005f;
        float attenuation_multiplier = 0.014f;
        float fog_factor = 20.f;

        Vector3 rgb_attenuation = Vector3(0.42f, 0.1f, 0.11f);
        int num_steps = 15;

        float max_distance = 1000.f;
    } underwater;

#if defined(_DEBUG)
    void imGuiConfig() {
        if (ImGui::BeginMenu("Rendering Parameters")) {
            ImGui::SliderFloat3("Sun Direction", Vec3ImGuiAddr(sun_direction),
                                -5.f, 5.f);
            sun_direction.inplaceNormalize();
            ImGui::SliderFloat3("Sun Color", Vec3ImGuiAddr(sun_color), 0.0f,
                                1.f);

            ImGui::SeparatorText("Underwater Parameters");
            ImGui::SliderFloat("Sky Multiplier", &underwater.sky_multiplier,
                               0.0f, 1.f);
            ImGui::SliderFloat("Scattering Multiplier",
                               &underwater.scattering_multiplier, 0.0f, 0.01f);
            ImGui::SliderFloat("Attenation Multiplier",
                               &underwater.attenuation_multiplier, 0.f, 0.03f);
            ImGui::SliderFloat("Fog", &underwater.fog_factor, 1.f, 30.f);
            ImGui::SliderFloat3(
                "RGB Attenuation",
                static_cast<float*>(&underwater.rgb_attenuation.x), 0.0f, 1.f);
            ImGui::SliderInt("Num Steps", &underwater.num_steps, 3, 30);
            ImGui::SliderFloat("Max Distance", &underwater.max_distance, 500,
                               10000.f);

            ImGui::EndMenu();
        }
    }
#endif
};

#if defined(_DEBUG)
// ImGuiConfig:
// Sets configuration parameters
void VisualSystem::imGuiConfig() { config->imGuiConfig(); }
#endif

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

    resource_manager = NULL;
    pipeline = NULL;

    camera = nullptr;
    terrain = nullptr;

    // Connect to Datamodel
    DMCamera::ConnectToCreation(
        [this](Object* obj) { this->onObjectCreate(obj); });

    camera = nullptr;

    // Initialize my pipeline
    HRESULT result;
    pipeline = new Pipeline(window);

    device = pipeline->getDevice();
    context = pipeline->getContext();

    {
        D3D11_RASTERIZER_DESC rast_desc = {};
        rast_desc.FillMode = D3D11_FILL_SOLID;
        rast_desc.CullMode = D3D11_CULL_FRONT;
        rast_desc.FrontCounterClockwise = 0;
        result = device->CreateRasterizerState(&rast_desc, &rast_state);
        assert(SUCCEEDED(result));
    }

    // Initialize each of my managers with the resources they need
    resource_manager = new ResourceManager(device, context);
    resource_manager->initializeResources();

    light_manager = new LightManager(device, 4096);
}

// --- Component Bindings ---
void VisualSystem::onObjectCreate(Object* object) {
    const uint16_t class_id = object->getClassID();

    if (class_id == Object::GetObjectClassIDByName("Camera")) {
        camera.reset(new Camera(object));
    }
}

// Render:
// Renders the entire scene to the screen.
void VisualSystem::render() {
    pipeline->prepare();

#if defined(_DEBUG)
    ICPUTimer cpu_timer = CPUTimer::TrackCPUTime("CPU Frametime");
    {
        IGPUTimer gpu_timer = GPUTimer::TrackGPUTime("GPU Frametime");
#endif

        // TEMP
        // light_manager->updateTimeOfDay(15.f);

        // Upload CB0
        {
            IConstantBuffer pcb0_common = pipeline->loadPixelCB(CB0);

            const Vector3 cam_pos = camera->getPosition();
            const Vector3 cam_direc = camera->forward();
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
        performPrepass(); //..

        // Bind my atlases
        resource_manager->getColorAtlas()->PSBindResource(context, 0);
        light_manager->getAtlasTexture()->PSBindResource(context, 1);

        // Render terrain
        performTerrainPass();
        // Render meshes
        performRenderPass();

        // Rendering is different depending on if we're below
        // or above the surface of the water
        const Vector3& cam_pos = camera->getPosition();
        if (cam_pos.y <= terrain->getSurfaceLevel() + 6.5f) {
            // Underwater rendering
            processUnderwater();
            performLightFrustumPass();
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

#if defined(_DEBUG)
    }
#endif

    // Finish rendering and present
    pipeline->present();
}

// PullSceneData:
// Prepares the engine for rendering, by pulling all necessary
// data from the datamodel.
void VisualSystem::pullSceneData(Scene* scene) {
#if defined(_DEBUG)
    ICPUTimer cpu_timer = CPUTimer::TrackCPUTime("Render Prepare");
#endif

#if defined(_DEBUG)
    imGuiConfig();
#endif

    // Pull Datamodel Data
    camera.get()->pullDatamodelData();

    // Pull my terrain data (if it exists).
    Terrain* scene_terrain = scene->getTerrain();

    if (scene_terrain == nullptr) {
        if (terrain != nullptr) {
            delete terrain;
            terrain = nullptr;
        }
    } else {
        if (terrain == nullptr)
            terrain = new VisualTerrain(scene_terrain, device);

        terrain->pullTerrainMeshes(context);
    }

    // Pull my object data.
    // Remove invalid visual objects, and update them to pull
    // the datamodel data.
    // asset_components.cleanAndUpdate();
    light_manager->pullDatamodelData();

    // Prepare managers for data
    light_manager->updateSunDirection(config->sun_direction);
    light_manager->updateSunCascades(camera->frustum());

    light_manager->resetShadowCasters();

    for (const AssetComponent* object : asset_components) {
        const Asset* asset = object->getAsset();

        if (asset->isSkinned())
            asset->applyAnimationAtTime(1, cache->time);

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
    static BumpMapBuilder bump_builder = BumpMapBuilder(size, size);

    static float freq = 0.02f;
    static float AMP = 2.f;

#if defined(_DEBUG)
    if (ImGui::BeginMenu("Misc")) {
        ImGui::SliderFloat("Frequency", &freq, 0.001f, 0.5f);
        ImGui::SliderFloat("Amplitude", &AMP, 1.f, 25.f);

        ImGui::EndMenu();
    }
#endif

    if (bump_tex == nullptr) {
        bump_builder.samplePerlinNoise(120512, freq, AMP);

        if (bump_tex != nullptr) {
            bump_builder.update(bump_tex, context);
        } else
            bump_tex = bump_builder.generate(device, true);
    }

    // bump_tex->displayImGui();

    // ---

    // TODO: Terrain is not included in shadows
    /*BufferPool* bpool_terrain = terrain->getMesh();
    static Mesh* t_mesh = new Mesh();
    t_mesh->index_buffer = bpool_terrain->getIndexBuffer();
    t_mesh->triangle_count = bpool_terrain->getNumTriangles();
    t_mesh->vertex_streams[POSITION] = bpool_terrain->getPositionBuffer();
    t_mesh->aabb = AABB();
    t_mesh->aabb.expandToContain(Vector3(-500, -500, -500));
    t_mesh->aabb.expandToContain(Vector3(500, 500, 500));
    ShadowCaster shadowCaster;
    shadowCaster.mesh = t_mesh;
    shadowCaster.m_localToWorld = Matrix4::Identity();
    light_manager->addShadowCaster(shadowCaster);*/

    /*const std::vector<Mesh*> chunk_meshes = terrain->getMeshes();
    visible_chunks.clear();
    for (Mesh* mesh : chunk_meshes) {
        OBB obb = OBB(mesh->aabb, Matrix4::Identity());
        if (cam_frustum.intersectsOBB(obb))
            visible_chunks.push_back(mesh);
    }*/

    // Cluster shadows
    light_manager->clusterShadowCasters();

    // Update the values stored in my cache
    Texture* target = pipeline->getRenderTargetDest();

    cache->time += 1 / 60.f;
    cache->m_world_to_screen =
        camera->getFrustumMatrix() * camera->getWorldToCameraMatrix();
    cache->m_screen_to_world = cache->m_world_to_screen.inverse();
    cache->resolution_info =
        Vector4((float)target->width, (float)target->height, camera->getZNear(),
                camera->getZFar());
}

// PerformShadowPass:
// Render the scene from each light's point of view, to populate
// its shadow map.
void VisualSystem::performPrepass() {
#if defined(_DEBUG)
    IGPUTimer gpu_timer = GPUTimer::TrackGPUTime("Shadow Pass");
#endif

    pipeline->bindVertexShader("ShadowMap");
    pipeline->bindPixelShader("ShadowMap");

    const Texture* shadow_texture = light_manager->getAtlasTexture();
    shadow_texture->clearAsDepthStencil(context);

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

            pipeline->drawMesh(mesh, INDEX_LIST_START, INDEX_LIST_END, 1);
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
    Texture* depth_stencil = pipeline->getDepthStencil();
    pipeline->bindRenderTarget(Target_UseExisting, Depth_TestAndWrite,
                               Blend_Default);
    depth_stencil->clearAsDepthStencil(context);

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

    context->IASetIndexBuffer(NULL, DXGI_FORMAT_R32_UINT, 0);

    terrain->getDescriptorSB().VSBindResource(context, 0);
    terrain->getIndexSB().VSBindResource(context, 1);
    terrain->getPositionSB().VSBindResource(context, 2);
    terrain->getNormalSB().VSBindResource(context, 3);

    // We draw instanced without indices, so the index buffer has no influence
    // on the final result.
    const int num_chunks = terrain->getActiveChunkCount();
    const int max_tris = terrain->getMaxChunkTriangleCount();
    context->DrawInstanced(max_tris * 3, num_chunks, 0, 0);
}

void VisualSystem::performRenderPass() {
#if defined(_DEBUG)
    IGPUTimer gpu_timer = GPUTimer::TrackGPUTime("Render Pass");
#endif

    pipeline->bindPixelShader("TexturedMesh");
    pipeline->bindRenderTarget(Target_UseExisting, Depth_TestAndWrite,
                               Blend_Default);

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
    for (const AssetComponent* comp : asset_components) {
        const Asset* asset = comp->getAsset();
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
            const Matrix4& mLocalToWorld = comp->getLocalToWorldMatrix();
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
                        // TODO: THIS IS BOTTLE NECKING MY CODE
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
            pipeline->drawMesh(mesh, INDEX_LIST_START, INDEX_LIST_END, 1);
        }
    }
}

void VisualSystem::performLightFrustumPass() {
#if defined(_DEBUG)
    IGPUTimer gpu_timer = GPUTimer::TrackGPUTime("Light Frustum Pass");
#endif

    const std::vector<ShadowLight*>& lights = light_manager->getShadowLights();

    if (lights.size() <= SUN_NUM_CASCADES)
        return;

    const int num_lights = lights.size() - SUN_NUM_CASCADES;

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
        }
    }

    {
        IConstantBuffer pCB2 = pipeline->loadPixelCB(CB2);

        const Vector3 sun_direc = config->sun_direction.unit();
        pCB2.loadData(&sun_direc, FLOAT3);
        const float surface_height = terrain->getSurfaceLevel() + 3.f;
        pCB2.loadData(&surface_height, FLOAT);

        const auto& underwater_params = config->underwater;
        pCB2.loadData(&underwater_params.sky_multiplier, FLOAT);
        pCB2.loadData(&underwater_params.scattering_multiplier, FLOAT);
        pCB2.loadData(&underwater_params.attenuation_multiplier, FLOAT);
        pCB2.loadData(&underwater_params.fog_factor, FLOAT);
        pCB2.loadData(&underwater_params.rgb_attenuation, FLOAT3);
        pCB2.loadData(&underwater_params.num_steps, INT);
        pCB2.loadData(&underwater_params.max_distance, FLOAT);

        static float temp = 0.05f;
        static float temp2 = 1.f;
        if (ImGui::BeginMenu("Misc")) {
            ImGui::SliderFloat("Multiplier", &temp, 0.f, 0.5f);
            ImGui::SliderFloat("Multiplier2", &temp2, 0.f, 3000.f);
            ImGui::EndMenu();
        }
        pCB2.loadData(&temp, FLOAT);
        pCB2.loadData(&temp2, FLOAT);
    }

    // TODO:
    // All frustums render to the same spot in the depth buffer, which can
    // exaggerate the total light reaching the camera. They should instead
    // render to different parts, kind of like a shadow map.

    // Render front of frustum
    pipeline->bindVertexShader("LightFrustum");
    pipeline->bindPixelShader("PreLightFrustum");
    pipeline->bindRenderTarget(Target_Disabled, Depth_TestAndWrite,
                               Blend_Default);

    const Texture* depth_copy = pipeline->getDepthStencilCopy();
    depth_copy->clearAsDepthStencil(context);
    ID3D11RenderTargetView* render_view =
        pipeline->getRenderTargetDest()->target_view;
    ID3D11DepthStencilView* depth_view = depth_copy->depth_view;
    context->OMSetRenderTargets(0, nullptr, depth_view);

    Asset* frustum_cube = resource_manager->getAsset("Cube");
    const Mesh* cube_mesh = frustum_cube->getMesh(0);

    pipeline->drawMesh(cube_mesh, INDEX_LIST_START, INDEX_LIST_END, num_lights);

    // Render the Rest
    pipeline->bindVertexShader("LightFrustum");
    pipeline->bindPixelShader("LightFrustum");
    pipeline->bindRenderTarget(Target_UseExisting, Depth_Disabled,
                               Blend_UseSrcAndDest);
    context->RSSetState(rast_state);

    pipeline->bindDepthStencil(3);
    context->PSSetShaderResources(4, 1, &depth_copy->shader_view);

    pipeline->drawMesh(cube_mesh, INDEX_LIST_START, INDEX_LIST_END, num_lights);

    context->RSSetState(NULL);
}

void VisualSystem::performWaterSurfacePass() {
#if defined(_DEBUG)
    IGPUTimer gpu_timer = GPUTimer::TrackGPUTime("Water Surface Pass");
#endif

    pipeline->bindVertexShader("WaterSurface");
    pipeline->bindPixelShader("WaterSurface");

    // Copy depth stencil over
    Texture* depth_stencil = pipeline->getDepthStencil();
    Texture* depth_stencil_copy = pipeline->getDepthStencilCopy();
    context->CopyResource(depth_stencil_copy->texture, depth_stencil->texture);

    // Disable depth writes
    pipeline->bindRenderTarget(Target_UseExisting, Depth_TestAndWrite,
                               Blend_SrcAlphaOnly);

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

    const Mesh* surface_mesh = terrain->getWaterSurface()->getSurfaceMesh();
    const int total_triangles = surface_mesh->num_triangles;
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
    pipeline->bindRenderTarget(Target_SwapTarget, Depth_Disabled,
                               Blend_Default);

    // Set samplers and texture
    pipeline->bindInactiveTarget(2);
    pipeline->bindDepthStencil(3);

    {
        IConstantBuffer pcb2 = pipeline->loadPixelCB(CB2);

        pcb2.loadData(&config->sun_direction, FLOAT3);
        const float sun_size = 0.0125f;
        pcb2.loadData(&sun_size, FLOAT);
        pcb2.loadData(&config->sun_color, FLOAT3);
        pcb2.loadData(nullptr, FLOAT);

        static float density_falloff = 8.f;
        pcb2.loadData(&density_falloff, FLOAT);
        static float atmosphere_height = 500.f;
        pcb2.loadData(&atmosphere_height, FLOAT);
        static float max_distance = 1000.f;
        pcb2.loadData(&max_distance, FLOAT);
        static int num_steps_atmosphere = 8;
        pcb2.loadData(&num_steps_atmosphere, INT);

        static float scattering = 0.135f;
        const Vector3 scattering_coefficients =
            Vector3(powf(200.f / 700.f, 4), powf(200.f / 530.f, 4),
                    powf(200.f / 440.f, 4)) *
            scattering;
        pcb2.loadData(&scattering_coefficients, FLOAT3);
        static int num_steps_optical_depth = 8;
        pcb2.loadData(&num_steps_optical_depth, INT);

        static float reflective_strength = 1.f;
        pcb2.loadData(&reflective_strength, FLOAT);

#if defined(_DEBUG)
        if (ImGui::BeginMenu("Misc")) {
            ImGui::SliderFloat("Density Falloff", &density_falloff, 0.f, 8.f);
            ImGui::SliderFloat("Atmosphere Height", &atmosphere_height, 0.f,
                               500.f);
            ImGui::SliderFloat("Max Distance", &max_distance, 0.f, 1000.f);
            ImGui::SliderFloat("Scattering", &scattering, 0.0f, 0.5f);

            ImGui::SliderInt("Steps Atmosphere", &num_steps_atmosphere, 0, 20);
            ImGui::SliderInt("Steps Optical Depth", &num_steps_optical_depth, 0,
                             20);
            ImGui::SliderFloat("Reflective Strength", &reflective_strength, 0.f,
                               2.f);
            ImGui::EndMenu();
        }
#endif
    }

    pipeline->drawPostProcessQuad();
}

void VisualSystem::processUnderwater() {
#if defined(_DEBUG)
    IGPUTimer gpu_timer = GPUTimer::TrackGPUTime("Underwater Pass");
#endif

    pipeline->bindVertexShader("PostProcess");
    pipeline->bindPixelShader("Underwater");
    pipeline->bindRenderTarget(Target_SwapTarget, Depth_Disabled,
                               Blend_Default);

    // Set samplers and texture resources
    pipeline->bindInactiveTarget(2);
    pipeline->bindDepthStencil(3);

    // Set parameters
    {
        IConstantBuffer pCB2 = pipeline->loadPixelCB(CB2);

        const Vector3 sun_direc = config->sun_direction.unit();
        pCB2.loadData(&sun_direc, FLOAT3);
        const float surface_height = terrain->getSurfaceLevel() + 3.f;
        pCB2.loadData(&surface_height, FLOAT);

        const auto& underwater_params = config->underwater;
        pCB2.loadData(&underwater_params.sky_multiplier, FLOAT);
        pCB2.loadData(&underwater_params.scattering_multiplier, FLOAT);
        pCB2.loadData(&underwater_params.attenuation_multiplier, FLOAT);
        pCB2.loadData(&underwater_params.fog_factor, FLOAT);
        pCB2.loadData(&underwater_params.rgb_attenuation, FLOAT3);
        pCB2.loadData(&underwater_params.num_steps, INT);
        pCB2.loadData(&underwater_params.max_distance, FLOAT);
    }

    // Bind and draw full screen quad
    pipeline->drawPostProcessQuad();
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

    ID3D11Buffer* indexBuffer = mesh->buffer_pool->ibuffer;
    ID3D11Buffer* vertexBuffer = mesh->buffer_pool->vbuffers[POSITION];
    int numIndices = mesh->num_triangles * 3;

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

} // namespace Graphics
} // namespace Engine