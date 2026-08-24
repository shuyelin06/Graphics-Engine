#include "VisualSystem.h"

#include "Direct3D11.h"

#include "util/RenderDoc.h"

#if defined(_DEBUG)
#include "util/CPUTimer.h"
#endif

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

    ImGuiHelper::registerImGuiCallback("Render/Core", [this]() { doCoreUI(); });
    ImGuiHelper::registerImGuiCallback("Render/Renderdoc",
                                       [this]() { doRenderDocUI(); });
}

// Render:
// Renders the entire scene to the screen.
void VisualSystem::render()
{
    terrain2D->updatePerform(context);

    context->beginFrame(frame++);

    pipeline->beginFrame(frame++);

#if defined(_DEBUG)
    {
        ICPUTimer cpu_timer = CPUTimer::TrackCPUTime("CPU Frametime");
#endif

        context->beginPass("Pass 1");
        render_manager->perform();
        context->endPass();

        context->beginPass("Pass 2");
        visual_debug->render(context);
        context->endPass();

        postfx_manager->render(context);

#if defined(_DEBUG)
    }
#endif

    // Finish rendering and present
    pipeline->endFrame();

    context->endFrame();

    // Finish RenderDoc Capture (if initialized and we are taking one)
    RenderDoc::EndRenderDocCaptureIfCapturing();
}

void VisualSystem::renderPrepare()
{
#if defined(_DEBUG)
    ICPUTimer cpu_timer = CPUTimer::TrackCPUTime("Render Prepare");
#endif

#if defined(IMGUI_ENABLED)
    ImGuiHelper::renderImGui();
#endif

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

Device* VisualSystem::getDevice() const { return device; }
ResourceManager* VisualSystem::getResourceManager() const
{
    return resource_manager.get();
}
MaterialManager* VisualSystem::getMaterialManager() const
{
    return material_manager.get();
}
SceneListener* VisualSystem::getSceneListener() const
{
    return scene_listener.get();
}
SceneManager* VisualSystem::getSceneManager() const
{
    return scene_manager.get();
}
RenderManager* VisualSystem::getRenderManager() const
{
    return render_manager.get();
}
LightManager* VisualSystem::getLightManager() const { return light_manager; }

Pipeline* VisualSystem::getPipeline() const { return pipeline.get(); }

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