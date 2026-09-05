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
    // Initialize my pipeline
    pipeline = std::make_unique<Pipeline>(window);

    device = pipeline->getDevice();
    ID3D11Device* deviceInterface = device->getDevice();
    context = pipeline->getContext();

    resource_manager = ResourceManager::create(device, context);
    resource_manager->initializeSystemResources();
    material_manager = MaterialManager::create(resource_manager.get());

    visual_debug =
        std::make_unique<VisualDebug>(device, resource_manager->getCubeMesh());

    render_manager =
        RenderManager::create(this, context->getContext(), deviceInterface);
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

// Render:
// Renders the entire scene to the screen.
void VisualSystem::render()
{
    {
        PROFILE_SCOPE("TEST");

        terrain2D->updatePerform(context);

        beginRenderFrame();

        {
            PROFILE_SCOPE("Render Manager");
            context->beginPass("Pass 1");
            render_manager->perform();
            context->endPass();
        }

        {
            PROFILE_SCOPE("Debug Render");
            context->beginPass("Debug Render");
            visual_debug->render(context);
            context->endPass();
        }

        postfx_manager->render(context);
    }

    endRenderFrame();
}

void VisualSystem::beginRenderFrame()
{
    frame++;
    context->beginFrame(frame);
    pipeline->beginFrame(frame);
}

void VisualSystem::endRenderFrame()
{
    pipeline->endFrame();
    context->endFrame();

    // Swapchain present
    // Finished presenting so finish
    // RenderDoc Capture (if initialized and we are taking one)
    context->present();
    RenderDoc::EndRenderDocCaptureIfCapturing();

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
    terrain2D->update(scene_manager->getMainCamera()->getPosition());

    // Prepare managers for data
    light_manager->updateSunDirection(Vector3(0, -1, 0));
    light_manager->updateSunCascades(scene_manager->getMainCamera()->frustum());
    light_manager->resetShadowCasters();
    light_manager->clusterShadowCasters();

    // Serve Resource Requests
    resource_manager->updatePerform();

    Camera* camera = scene_manager->getMainCamera();
    std::shared_ptr<Texture> target = pipeline->getRenderTargetDest();
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
    mainView.renderTarget = target;
    mainView.depthStencil = pipeline->getDepthStencil();
    render_manager->setMainView(mainView);
}

void VisualSystem::doCoreUI()
{
#if defined(IMGUI_ENABLED)
    if (ImGui::Button("Reload Shaders"))
    {
        device->reloadShaders();
    }

    if (ImGui::CollapsingHeader("GPU Frametime"))
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

    if (ImGui::Button("Take RenderDoc Capture"))
    {
        RenderDoc::StartRenderDocCapture();
    }
#endif
}

} // namespace Graphics
} // namespace Engine