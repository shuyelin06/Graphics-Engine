#pragma once

#include <cstdint>

#include "VisualDebug.h"
#include "core/Device.h"
#include "lights/LightManager.h"
#include "pipeline/RenderManager.h"
#include "postfx/PostFXManager.h"
#include "resources/MaterialManager.h"
#include "resources/ResourceManager.h"
#include "scene/SceneListener.h"
#include "scene/SceneManager.h"
#include "terrain2D/Terrain2DManager.h"

struct ID3D11Device;
struct ID3D11DeviceContext;

namespace Engine
{
using namespace Datamodel;

namespace Graphics
{
class Texture;

class SceneListener;
class SceneManager;
class ResourceManager;
class MaterialManager;
class PostFXManager;
class VisualDebug;

struct MainRenderTargets
{
    std::shared_ptr<Texture> render_target_dest = nullptr;
    std::shared_ptr<Texture> render_target_src = nullptr;

    std::shared_ptr<Texture> depth_stencil = nullptr;

    inline void swapActiveRenderTarget()
    {
        std::swap(render_target_dest, render_target_src);
    }
};

// VisualSystem Class:
// Provides an interface for the application's graphics.
// VisualSystem is in charge of the different rendering passes;
// pipeline provides a convenient interface for some functionality.
class VisualSystem
{
  private:
    // Frame
    uint64_t frame;

    // Managers
    std::unique_ptr<Device> device;
    std::unique_ptr<DeviceContext> context;

    std::unique_ptr<MainRenderTargets> render_targets;

    std::unique_ptr<VisualDebug> visual_debug;
    std::unique_ptr<ResourceManager> resource_manager;
    std::unique_ptr<MaterialManager> material_manager;
    std::unique_ptr<RenderManager> render_manager;
    std::unique_ptr<PostFXManager> postfx_manager;

    std::unique_ptr<SceneListener> scene_listener;
    std::unique_ptr<SceneManager> scene_manager;
    std::unique_ptr<Terrain2DManager> terrain2D;
    LightManager* light_manager;

  public:
    VisualSystem(HWND window);

    // Call these functions to render the scene. Renders an entire scene
    void renderPrepare();
    void renderPerform();

    // clang-format off
    Device* getDevice() const { return device.get(); }
    ResourceManager* getResourceManager() const { return resource_manager.get(); }
    MaterialManager* getMaterialManager() const { return material_manager.get(); }
    SceneListener* getSceneListener() const { return scene_listener.get(); }
    SceneManager* getSceneManager() const { return scene_manager.get(); }
    RenderManager* getRenderManager() const { return render_manager.get(); }
    LightManager* getLightManager() const { return light_manager; }
    VisualDebug* getVisualDebug() const { return visual_debug.get(); };
    MainRenderTargets* getMainRenderTargets() const { return render_targets.get(); }
    // clang-format on

  private:
    void beginRenderFrame();
    void endRenderFrame();

    void doCoreUI();
    void doRenderDocUI();
};
} // namespace Graphics
} // namespace Engine