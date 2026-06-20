#include "PostFXManager.h"

#include "rendering/pipeline/PipelineManager.h"

#include "rendering/ImGui.h"

#if defined(_DEBUG)
#include "rendering/util/CPUTimer.h"
#include "rendering/util/GPUTimer.h"
#endif

namespace Engine {
namespace Graphics {
struct SkyConfig {
    bool renderSky = true;

    float density_falloff = 8.f;
    float atmosphere_height = 500.f;
    float max_distance = 1000.f;
    int num_steps_atmosphere = 8;
    float scattering = 0.135f;
    int num_steps_optical_depth = 8;
    float reflective_strength = 1.f;
};

class PostFXManagerImpl {
  private:
    VisualSystem* mVisualSystem;

  public:
    PostFXManagerImpl(VisualSystem* visualSystem);

    void render();

  private:
    SkyConfig mSkyConfig;

    void renderSky(Pipeline* pipeline);

    void imGui();
};

std::unique_ptr<PostFXManager>
PostFXManager::create(VisualSystem* visualSystem) {
    std::unique_ptr<PostFXManager> ptr =
        std::unique_ptr<PostFXManager>(new PostFXManager());
    ptr->mImpl = std::make_unique<PostFXManagerImpl>(visualSystem);
    return ptr;
}
PostFXManager::PostFXManager() = default;
PostFXManager::~PostFXManager() = default;

void PostFXManager::render() { mImpl->render(); }

PostFXManagerImpl::PostFXManagerImpl(VisualSystem* visualSystem)
    : mVisualSystem(visualSystem) {

    ImGuiHelper::registerImGuiCallback("Render/PostFX", [this]() { imGui(); });
}

void PostFXManagerImpl::render() {
    Pipeline* pipeline = mVisualSystem->getPipeline();

    if (mSkyConfig.renderSky) {
        renderSky(pipeline);
    }
}

void PostFXManagerImpl::renderSky(Pipeline* pipeline) {
#if defined(_DEBUG)
    IGPUTimer gpu_timer = GPUTimer::TrackGPUTime("Sky Processing");
#endif

    pipeline->bindVertexShader("PostProcess");
    pipeline->bindPixelShader("Sky");
    pipeline->bindRenderTarget(Target_SwapTarget, Depth_Disabled,
                               Blend_Default);

    // Set samplers and texture
    pipeline->bindPixelTexture(2, *pipeline->getRenderTargetSrc(),
                               SamplerType::Sampler_Point);
    pipeline->bindPixelTexture(3, *pipeline->getDepthStencil(),
                               SamplerType::Sampler_Point);

    {
        IConstantBuffer pcb2 = pipeline->loadPixelCB(2);

        Vector3 sun_direction = Vector3(-3.0f, -1.0f, 0.0f).unit();
        pcb2.loadData(&sun_direction, FLOAT3);
        const float sun_size = 0.0125f;
        pcb2.loadData(&sun_size, FLOAT);
        Vector3 sun_color = Vector3(1.f, 1.f, 0.0f);
        pcb2.loadData(&sun_color, FLOAT3);
        pcb2.loadData(nullptr, FLOAT);

        pcb2.loadData(&mSkyConfig.density_falloff, FLOAT);
        pcb2.loadData(&mSkyConfig.atmosphere_height, FLOAT);
        pcb2.loadData(&mSkyConfig.max_distance, FLOAT);
        pcb2.loadData(&mSkyConfig.num_steps_atmosphere, INT);

        const Vector3 scattering_coefficients =
            Vector3(powf(200.f / 700.f, 4), powf(200.f / 530.f, 4),
                    powf(200.f / 440.f, 4)) *
            mSkyConfig.scattering;
        pcb2.loadData(&scattering_coefficients, FLOAT3);
        pcb2.loadData(&mSkyConfig.num_steps_optical_depth, INT);

        pcb2.loadData(&mSkyConfig.reflective_strength, FLOAT);
    }

    pipeline->drawPostProcessQuad();
}

void PostFXManagerImpl::imGui() {
#if defined(IMGUI_ENABLED)
    ImGui::Checkbox("Render Sky", &mSkyConfig.renderSky);
    if (mSkyConfig.renderSky) {
        if (ImGui::CollapsingHeader("Sky Config")) {
            ImGui::SliderFloat("Density Falloff", &mSkyConfig.density_falloff,
                               0.f, 8.f);
            ImGui::SliderFloat("Atmosphere Height",
                               &mSkyConfig.atmosphere_height, 0.f, 500.f);
            ImGui::SliderFloat("Max Distance", &mSkyConfig.max_distance, 0.f,
                               1000.f);
            ImGui::SliderFloat("Scattering", &mSkyConfig.scattering, 0.0f,
                               0.5f);

            ImGui::SliderInt("Steps Atmosphere",
                             &mSkyConfig.num_steps_atmosphere, 0, 20);
            ImGui::SliderInt("Steps Optical Depth",
                             &mSkyConfig.num_steps_optical_depth, 0, 20);
            ImGui::SliderFloat("Reflective Strength",
                               &mSkyConfig.reflective_strength, 0.f, 2.f);
        }
    }
#endif
}
} // namespace Graphics
} // namespace Engine