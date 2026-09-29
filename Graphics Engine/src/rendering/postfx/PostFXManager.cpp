#include "PostFXManager.h"

#include "rendering/ImGui.h"

namespace Engine
{
namespace Graphics
{
struct SkyConfig
{
    bool renderSky = false;

    float density_falloff = 8.f;
    float atmosphere_height = 500.f;
    float max_distance = 1000.f;
    int num_steps_atmosphere = 8;
    float scattering = 0.135f;
    int num_steps_optical_depth = 8;
    float reflective_strength = 1.f;
};

class PostFXManagerImpl
{
  private:
    VisualSystem* mVisualSystem;

    std::shared_ptr<Geometry> postprocessQuad = nullptr;

  public:
    PostFXManagerImpl(VisualSystem* visualSystem);

    void render(DeviceContext* context);
    void drawPostFXQuad(DeviceContext* context);

  private:
    SkyConfig mSkyConfig;

    void renderSky(DeviceContext* context);

    void imGui();
};

std::unique_ptr<PostFXManager> PostFXManager::create(VisualSystem* visualSystem)
{
    std::unique_ptr<PostFXManager> ptr =
        std::unique_ptr<PostFXManager>(new PostFXManager());
    ptr->mImpl = std::make_unique<PostFXManagerImpl>(visualSystem);
    return ptr;
}
PostFXManager::PostFXManager() = default;
PostFXManager::~PostFXManager() = default;

void PostFXManager::render(DeviceContext* context) { mImpl->render(context); }

void PostFXManager::drawPostFXQuad(DeviceContext* context)
{
    mImpl->drawPostFXQuad(context);
}

PostFXManagerImpl::PostFXManagerImpl(VisualSystem* visualSystem)
    : mVisualSystem(visualSystem)
{
    {
        Device* device = mVisualSystem->getDevice();

        const Vector4 fullscreen_quad[6] = {
            // First Triangle
            Vector4(-1, -1, 0, 1), Vector4(-1, 1, 0, 1), Vector4(1, 1, 0, 1),
            // Second Triangle
            Vector4(-1, -1, 0, 1), Vector4(1, 1, 0, 1), Vector4(1, -1, 0, 1)};

        std::shared_ptr<Buffer> fullScreenQuad = device->createBuffer(
            "Full Screen Quad", BufferType::Vertex, sizeof(fullscreen_quad),
            fullscreen_quad, false);

        postprocessQuad = std::make_shared<Geometry>();
        postprocessQuad->vertexBuffers[VertexDataStream::PosXYZ_TexU] =
            fullScreenQuad;
        postprocessQuad->indexCount = 6;
    }

    ImGuiHelper::RegisterImGuiCallback("Render/PostFX", [this]() { imGui(); });
}

void PostFXManagerImpl::render(DeviceContext* context)
{
    if (mSkyConfig.renderSky)
    {
        renderSky(context);
    }
}

void PostFXManagerImpl::drawPostFXQuad(DeviceContext* context)
{
    context->getContext()->IASetPrimitiveTopology(
        D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    context->draw(postprocessQuad.get(), 1);
}

void PostFXManagerImpl::renderSky(DeviceContext* context)
{
    MainRenderTargets* renderTargets = mVisualSystem->getMainRenderTargets();

    renderTargets->swapActiveRenderTarget();

    context->bindShaderProgram("PostProcess", "Sky");
    context->bindRenderTarget(renderTargets->render_target_dest, nullptr,
                              DepthSettings::Depth_Disabled,
                              BlendSettings::Blend_Default);

    // Set samplers and texture
    context->bindPixelTexture(2, renderTargets->render_target_src,
                              SamplerSettings::Point);
    context->bindPixelTexture(3, renderTargets->depth_stencil,
                              SamplerSettings::Point);

    {
        std::vector<uint8_t> data;
        auto appendData = [&data](const void* src, size_t bytes) {
            // Convert our data into a character array, and read the number of bytes
            // specified by the CBDataFormat into our constant buffer.
            const char* charData = static_cast<const char*>(src);

            data.resize(data.size() + bytes);
            uint8_t* vectorEnd = &data[data.size() - bytes];

            if (src != nullptr)
                memcpy(vectorEnd, charData, bytes);
            else
                memset(vectorEnd, 0, bytes);
        };

        Vector3 sun_direction = Vector3(-3.0f, -1.0f, 0.0f).unit();
        appendData(&sun_direction, 12);
        const float sun_size = 0.0125f;
        appendData(&sun_size, 4);
        Vector3 sun_color = Vector3(1.f, 1.f, 0.0f);
        appendData(&sun_color, 12);
        appendData(nullptr, 4);

        appendData(&mSkyConfig.density_falloff, 4);
        appendData(&mSkyConfig.atmosphere_height, 4);
        appendData(&mSkyConfig.max_distance, 4);
        appendData(&mSkyConfig.num_steps_atmosphere, 4);

        const Vector3 scattering_coefficients =
            Vector3(powf(200.f / 700.f, 4), powf(200.f / 530.f, 4),
                    powf(200.f / 440.f, 4)) *
            mSkyConfig.scattering;
        appendData(&scattering_coefficients, 12);
        appendData(&mSkyConfig.num_steps_optical_depth, 4);

        appendData(&mSkyConfig.reflective_strength, 4);

        context->loadPixelCB(2, data.data(), data.size());
    }

    drawPostFXQuad(context);
}

void PostFXManagerImpl::imGui()
{
#if defined(IMGUI_ENABLED)
    ImGui::Checkbox("Render Sky", &mSkyConfig.renderSky);
    if (mSkyConfig.renderSky)
    {
        if (ImGui::CollapsingHeader("Sky Config"))
        {
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