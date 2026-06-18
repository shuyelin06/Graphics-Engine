#include "VisualSystem.h"

#include <assert.h>
#include <d3d11_1.h>

#include "Direct3D11.h"

#include "resources/MaterialManager.h"
#include "resources/ResourceManager.h"
#include "scene/SceneListener.h"
#include "scene/SceneManager.h"

#include "VisualDebug.h"
#include "core/Frustum.h"
#include "datamodel/Object.h"

#include "core/Material.h"
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

// Constructor
// Initializes the VisualSystem
VisualSystem::VisualSystem(HWND window) {
    config = new VisualParameters();

    time = 0.f;

    bump_tex = nullptr;

    resource_manager = NULL;
    pipeline = NULL;

    // Initialize my pipeline
    HRESULT result;
    pipeline = std::make_unique<Pipeline>(window);
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

    resource_manager = ResourceManager::create(device, context);
    resource_manager->initializeSystemResources();
    material_manager = MaterialManager::create(resource_manager.get());
    render_manager = RenderManager::create(this, context, device);

    // Initialize each of my managers with the resources they need
    scene_listener = SceneListener::create(this);
    scene_manager = SceneManager::create(this);
   
    

    water_surface = std::make_unique<WaterSurface>();
    water_surface->generateSurfaceMesh(this->getResourceManager(), 15);
    water_surface->generateWaveConfig(14);

    light_manager = new LightManager(device, 4096);
    terrain2D = Terrain2DManager::create(this);
}

constexpr float kWaterSurfaceLevel = 0.f;

// Render:
// Renders the entire scene to the screen.
void VisualSystem::render() {
    // Update Perform

    pipeline->beginFrame(frame++);

#if defined(_DEBUG)
    ICPUTimer cpu_timer = CPUTimer::TrackCPUTime("CPU Frametime");
    {
        IGPUTimer gpu_timer = GPUTimer::TrackGPUTime("GPU Frametime");
#endif

        // TEMP
        // light_manager->updateTimeOfDay(15.f);

        // Prepare the shadow maps
        performPrepass(); //..

        // This is the new rendering.
        // TODO Migrate everything to this.
        render_manager->perform();

        // Rendering is different depending on if we're below
        // or above the surface of the water
        Camera* camera = scene_manager->getMainCamera();
        const Vector3& cam_pos = camera->getPosition();
        if (cam_pos.y <= kWaterSurfaceLevel + 6.5f) {
            // Underwater rendering
            processUnderwater();
            performLightFrustumPass();
        } else {
            // Above water rendering
            performWaterSurfacePass();
            processSky();
        }

#if defined(_DEBUG)
    }
#endif

    // Finish rendering and present
    pipeline->endFrame();
}

void VisualSystem::renderPrepare() {
#if defined(_DEBUG)
    ICPUTimer cpu_timer = CPUTimer::TrackCPUTime("Render Prepare");
#endif

#if defined(IMGUI_ENABLED)
    ImGuiHelper::renderImGui();
    config->imGuiConfig();
#endif

    // Parse all datamodel update packets since the last frame and update my
    // rendering systems.
    scene_listener->update();

    scene_manager->update();

    light_manager->pullDatamodelData();
    terrain2D->update(scene_manager->getMainCamera()->getPosition());

    // Prepare managers for data
    light_manager->updateSunDirection(config->sun_direction);
    light_manager->updateSunCascades(scene_manager->getMainCamera()->frustum());
    light_manager->resetShadowCasters();
    light_manager->clusterShadowCasters();

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

    // Serve Resource Requests
    resource_manager->updatePerform();

    time += 1 / 60.f;

    Camera* camera = scene_manager->getMainCamera();
    Texture* target = pipeline->getRenderTargetDest();
    RenderView mainView;
    mainView.position = camera->getPosition();
    mainView.zNear = camera->getZNear();
    mainView.direction = camera->forward();
    mainView.zFar = camera->getZFar();
    mainView.mWorldToLocal = camera->getWorldToCameraMatrix();
    mainView.mLocalToFrustum = camera->getFrustumMatrix();
    mainView.viewport = Vector4((float)target->width, (float)target->height,
                                camera->getZNear(), camera->getZFar());
    mainView.renderTarget = target;
    mainView.depthStencil = pipeline->getDepthStencil();
    render_manager->setMainView(mainView);
}

ResourceManager* VisualSystem::getResourceManager() const {
    return resource_manager.get();
}
SceneListener* VisualSystem::getSceneListener() const {
    return scene_listener.get();
}
SceneManager* VisualSystem::getSceneManager() const {
    return scene_manager.get();
}
RenderManager* VisualSystem::getRenderManager() const {
    return render_manager.get();
}
LightManager* VisualSystem::getLightManager() const { return light_manager; }

Pipeline* VisualSystem::getPipeline() const { return pipeline.get(); }

// PerformShadowPass:
// Render the scene from each light's point of view, to populate
// its shadow map.
void VisualSystem::performPrepass() {
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
            IConstantBuffer vCB0 = pipeline->loadVertexCB(2);

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
                IConstantBuffer vCB1 = pipeline->loadVertexCB(1);
                vCB1.loadData(&mLocalToWorld, FLOAT4X4);
            }

            pipeline->drawMesh(mesh, 1);
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
        IConstantBuffer vcb0 = pipeline->loadVertexCB(0);

        // This matrix scales the unit cube to the frustum cube. The frustum
        // cube has x in [-1,1], y in [-1,1], z in [0,1].
        Camera* camera = scene_manager->getMainCamera();
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
        IConstantBuffer pCB2 = pipeline->loadPixelCB(2);

        const Vector3 sun_direc = config->sun_direction.unit();
        pCB2.loadData(&sun_direc, FLOAT3);
        const float surface_height = kWaterSurfaceLevel + 3.f;
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

    const std::shared_ptr<Mesh> cube_mesh =
        resource_manager->getMesh(SystemMesh_Cube);

    pipeline->drawMesh(cube_mesh.get(), num_lights);

    // Render the Rest
    pipeline->bindVertexShader("LightFrustum");
    pipeline->bindPixelShader("LightFrustum");
    pipeline->bindRenderTarget(Target_UseExisting, Depth_Disabled,
                               Blend_UseSrcAndDest);
    context->RSSetState(rast_state);

    pipeline->bindDepthStencil(3);
    context->PSSetShaderResources(4, 1, &depth_copy->shader_view);

    pipeline->drawMesh(cube_mesh.get(), num_lights);

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

    Camera* camera = scene_manager->getMainCamera();
    {
        IConstantBuffer vcb0 = pipeline->loadVertexCB(0);
        const Matrix4 m_camera_to_screen =
            camera->getFrustumMatrix() * camera->getWorldToCameraMatrix();
        vcb0.loadData(&m_camera_to_screen, FLOAT4X4);

        const Vector3& camera_pos = camera->getPosition();
        vcb0.loadData(&camera_pos, FLOAT3);

        const float surface_height = kWaterSurfaceLevel;
        vcb0.loadData(&surface_height, FLOAT);
    }

    // VCB1: Wave Information
    const std::vector<WaveConfig>& wave_config = water_surface->getWaveConfig();
    const int num_waves = water_surface->getNumWaves();
    {
        IConstantBuffer vcb1 = pipeline->loadVertexCB(1);

        vcb1.loadData(&time, FLOAT);
        vcb1.loadData(&num_waves, INT);
        vcb1.loadData(nullptr, FLOAT2);

        for (int i = 0; i < num_waves; i++) {
            vcb1.loadData(&wave_config[i].direction, FLOAT2);
            vcb1.loadData(&wave_config[i].period, FLOAT);
            vcb1.loadData(&wave_config[i].amplitude, FLOAT);
        }
    }

    {
        IConstantBuffer pcb2 = pipeline->loadPixelCB(2);

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
        IConstantBuffer vCB2 = pipeline->loadVertexCB(2);

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

    const Mesh* surface_mesh = water_surface->getSurfaceMesh();
    const int total_triangles = surface_mesh->num_triangles;
    const int num_inner_tri = water_surface->getNumInnerTriangles();

    pipeline->drawMesh(surface_mesh, 4, 0, num_inner_tri);
    pipeline->drawMesh(surface_mesh, NUM_LODS * 4, num_inner_tri,
                       total_triangles);
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
        IConstantBuffer pcb2 = pipeline->loadPixelCB(2);

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
        IConstantBuffer pCB2 = pipeline->loadPixelCB(2);

        const Vector3 sun_direc = config->sun_direction.unit();
        pCB2.loadData(&sun_direc, FLOAT3);
        const float surface_height = kWaterSurfaceLevel + 3.f;
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

} // namespace Graphics
} // namespace Engine