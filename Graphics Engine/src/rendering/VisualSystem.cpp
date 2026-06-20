#include "VisualSystem.h"

#include "Direct3D11.h"

#if defined(_DEBUG)
#include "util/CPUTimer.h"
#include "util/GPUTimer.h"
#endif

namespace Engine {

namespace Graphics {
// Constructor
// Initializes the VisualSystem
VisualSystem::VisualSystem(HWND window) {
    // Initialize my pipeline
    pipeline = std::make_unique<Pipeline>(window);

    ID3D11Device* device = pipeline->getDevice();
    ID3D11DeviceContext* context = pipeline->getContext();

    resource_manager = ResourceManager::create(device, context);
    resource_manager->initializeSystemResources();
    material_manager = MaterialManager::create(resource_manager.get());

    render_manager = RenderManager::create(this, context, device);
    postfx_manager = PostFXManager::create(this);

    // Initialize each of my managers with the resources they need
    scene_listener = SceneListener::create(this);
    scene_manager = SceneManager::create(this);

    light_manager = new LightManager(device, 4096);
    terrain2D = Terrain2DManager::create(this);
}

// Render:
// Renders the entire scene to the screen.
void VisualSystem::render() {
    pipeline->beginFrame(frame++);

#if defined(_DEBUG)
    {
        ICPUTimer cpu_timer = CPUTimer::TrackCPUTime("CPU Frametime");
        IGPUTimer gpu_timer = GPUTimer::TrackGPUTime("GPU Frametime");
#endif

        render_manager->perform();
        postfx_manager->render();

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
MaterialManager* VisualSystem::getMaterialManager() const {
    return material_manager.get();
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

} // namespace Graphics
} // namespace Engine