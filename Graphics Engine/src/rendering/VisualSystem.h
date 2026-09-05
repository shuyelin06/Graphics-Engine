#pragma once

#include <cstdint>

#include "VisualDebug.h"
#include "core/Device.h"
#include "lights/LightManager.h"
#include "pipeline/PipelineManager.h"
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
class SceneListener;
class SceneManager;
class ResourceManager;
class MaterialManager;
class PostFXManager;
class VisualDebug;

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
    Device* device;
    DeviceContext* context;

    std::unique_ptr<Pipeline> pipeline;
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
    void render();

    // clang-format off
    Device* getDevice() const { return device; }
    ResourceManager* getResourceManager() const { return resource_manager.get(); }
    MaterialManager* getMaterialManager() const { return material_manager.get(); }
    SceneListener* getSceneListener() const { return scene_listener.get(); }
    SceneManager* getSceneManager() const { return scene_manager.get(); }
    RenderManager* getRenderManager() const { return render_manager.get(); }
    LightManager* getLightManager() const { return light_manager; }
    Pipeline* getPipeline() const { return pipeline.get(); }
    VisualDebug* getVisualDebug() const { return visual_debug.get(); };
    // clang-format on

  private:
    void beginRenderFrame();
    void endRenderFrame();

    void doCoreUI();
    void doRenderDocUI();
};
} // namespace Graphics
} // namespace Engine