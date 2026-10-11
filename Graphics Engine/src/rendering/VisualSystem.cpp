#include "VisualSystem.h"

#include "util/Profiling.h"
#include "util/RenderDoc.h"

namespace Engine
{
namespace Graphics
{
// Constructor
// Initializes the VisualSystem
VisualSystem::VisualSystem(HWND window)
{
    // Initialize my Graphics API!
    InitializeGraphicsAPI(window, device, context);

    Device* device = this->device.get();
    DeviceContext* context = this->context.get();

    {
        RECT rect;
        GetClientRect(window, &rect);
        const UINT width = rect.right - rect.left;
        const UINT height = rect.bottom - rect.top;
        initializeMainRenderTargets(device, width, height);
    }

    resource_manager = ResourceManager::create(device);
    resource_manager->initializeSystemResources();
    material_manager = MaterialManager::create(resource_manager.get());

    visual_debug =
        std::make_unique<VisualDebug>(device, resource_manager->getCubeMesh());

    render_manager = RenderManager::create(this);
    postfx_manager = PostFXManager::create(this);

    // Initialize each of my managers with the resources they need
    scene_listener = SceneListener::create(this);
    scene_manager = SceneManager::create(this);

    light_manager = new LightManager(this, device, 4096);
    terrain2D = Terrain2DManager::create(this);

    ImGuiHelper::RegisterImGuiCallback("Render/Core", [this]() { doCoreUI(); });
    ImGuiHelper::RegisterImGuiCallback("Render/Renderdoc",
                                       [this]() { doRenderDocUI(); });
    ImGuiHelper::RegisterImGuiCallback("Profiler",
                                       []() { Profiling::DoProfilerImgui(); });
}

void VisualSystem::initializeMainRenderTargets(Device* device,
                                               unsigned int width,
                                               unsigned int height)
{
    render_targets = std::make_unique<MainRenderTargets>();

    render_targets->colorMain = device->createTexture(
        "Render Target Destination", TextureLayout::R8G8B8A8_UNORM,
        TextureUsage::RenderTarget | TextureUsage::ShaderResource, width,
        height);
    render_targets->colorMainTarget = device->createRenderTarget(
        render_targets->colorMain, RenderTargetType::Color);

    render_targets->colorAlt = device->createTexture(
        "Render Target Source", TextureLayout::R8G8B8A8_UNORM,
        TextureUsage::RenderTarget | TextureUsage::ShaderResource, width,
        height);
    render_targets->colorAltTarget = device->createRenderTarget(
        render_targets->colorAlt, RenderTargetType::Color);

    render_targets->depthMain = device->createTexture(
        "Depth Stencil", TextureLayout::R24_UNORM_G8_UINT,
        TextureUsage::DepthStencil | TextureUsage::ShaderResource, width,
        height);
    render_targets->depthTarget = device->createRenderTarget(
        render_targets->depthMain, RenderTargetType::DepthStencil);
}

// Render:
// Renders the entire scene to the screen.
void VisualSystem::renderPerform()
{
    // Update Performs
    terrain2D->updatePerform(scene_manager->getMainCamera()->getPosition(),
                             context.get());

    beginRenderFrame();

    context->toggleWireframe(wireframeToggle);

    {
        PROFILE_SCOPE("Render Manager");
        context->beginPass("Pass 1");
        render_manager->perform(context.get());
        context->endPass();
    }

    context->toggleWireframe(false);

    {
        PROFILE_SCOPE("Debug Render");
        context->beginPass("Debug Render");
        visual_debug->render(context.get());
        context->endPass();
    }

    postfx_manager->render(context.get());

    endRenderFrame();
}

void VisualSystem::beginRenderFrame()
{
    frame++;
    context->beginFrame(frame);

    // Clear my target color so we can start rendering the next frame
    const float baseColor[4] = {0.f, 0.f, 0.f, 1.f};
    context->clearRenderTarget(render_targets->colorMainTarget, baseColor);
}

void VisualSystem::endRenderFrame()
{
    // Copy main render target to screen buffer
    {
        context->beginPass("Render Target Copy");

        std::shared_ptr<RenderTarget> screenTarget = context->getScreenTarget();
        context->bindRenderTargets(screenTarget, nullptr,
                                   DepthSettings::Depth_Disabled,
                                   BlendSettings::SrcAlphaOnly);
        PostFXParams params;
        params.resolutionX = render_targets->colorMain->getWidth();
        params.resolutionY = render_targets->colorMain->getHeight();

        context->bindPixelTexture(0, render_targets->colorMain,
                                  SamplerSettings::Point);

        postfx_manager->renderScreenspacePass(context.get(), params,
                                              "PassthroughPS");
        context->endPass();
    }

    // End Frame + Present
    context->endFrame();

    // Start next frame
    PROFILE_RESET();
}

void VisualSystem::renderPrepare()
{
    // Parse all datamodel update packets since the last frame and update my
    // rendering systems.
    scene_listener->update();

    scene_manager->update();

    light_manager->pullDatamodelData();

    // Prepare managers for data
    light_manager->updateSunDirection(Vector3(0, -1, 0));
    light_manager->updateSunCascades(scene_manager->getMainCamera()->frustum());
    light_manager->resetShadowCasters();
    light_manager->clusterShadowCasters();

    // Serve Resource Requests
    resource_manager->updatePerform(context.get());

    Camera* camera = scene_manager->getMainCamera();
    std::shared_ptr<Texture> target = render_targets->colorMain;
    RenderView mainView;
    mainView.position = camera->getPosition();
    mainView.zNear = camera->getZNear();
    mainView.direction = camera->forward();
    mainView.zFar = camera->getZFar();
    mainView.mWorldToLocal = camera->getWorldToCameraMatrix();
    mainView.mLocalToFrustum = camera->getFrustumMatrix();
    mainView.viewport =
        Vector4((float)target->getWidth(), (float)target->getHeight(),
                camera->getZNear(), camera->getZFar());
    mainView.renderTarget = render_targets->colorMainTarget;
    mainView.depthStencil = render_targets->depthTarget;
    render_manager->setMainView(mainView);
}

void VisualSystem::doCoreUI()
{
#if defined(IMGUI_ENABLED)
    // Shader Compilation
    ImGui::SeparatorText("Shader Compilation");
    {
        static bool lastCompilationStatus = true;
        static float lastTimeElapsedMs = 0.f;
        if (ImGui::Button("Reload Shaders"))
        {
            auto startTime = std::chrono::high_resolution_clock::now();
            lastCompilationStatus = device->reloadShaders();
            auto endTime = std::chrono::high_resolution_clock::now();

            lastTimeElapsedMs =
                std::chrono::duration_cast<std::chrono::microseconds>(endTime -
                                                                      startTime)
                    .count() /
                1000.f;
        }

        ImGui::SameLine();
        if (lastCompilationStatus)
        {
            ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f),
                               "Success! Took %.2f Ms", lastTimeElapsedMs);
        }
        else
        {
            ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f),
                               "Shader Compilation Failed.");
        }
    }

    // WireFrame Toggle
    ImGui::SeparatorText("Debug Views");
    ImGui::Checkbox("Wireframe View", &wireframeToggle);

    // GPU Frametimes
    ImGui::SeparatorText("GPU Frametimes");
    {
        const PassStats& passStats = context->getPassStats();
        ImGui::Text("Pass Stats (Smoothed, Frame %zu): %.2f ms Total",
                    passStats.frame, passStats.totalFrameTime);
        for (const PassStats::PassInfo& info : passStats.stats)
        {
            ImGui::Text("%s %.2f ms", info.name.data(), info.frameTime);
        }
    }
#endif
}

void VisualSystem::doRenderDocUI()
{
#if defined(IMGUI_ENABLED)
    if (!RenderDoc::IsRenderDocInitialized())
    {
        ImGui::Text("RenderDoc failed to initialize.");
        return;
    }

    RenderDoc::EndRenderDocCaptureIfCapturing();
    if (ImGui::Button("Take RenderDoc Capture"))
    {
        RenderDoc::StartRenderDocCapture();
    }
#endif
}

} // namespace Graphics
} // namespace Engine