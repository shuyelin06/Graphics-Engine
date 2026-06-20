#pragma once

#include <cstdint>

#include "lights/LightManager.h"
#include "pipeline/PipelineManager.h"
#include "pipeline/RenderManager.h"
#include "postfx/PostFXManager.h"
#include "resources/MaterialManager.h"
#include "resources/ResourceManager.h"
#include "scene/SceneListener.h"
#include "scene/SceneManager.h"
#include "terrain2D/Terrain2DManager.h"

namespace Engine {
using namespace Datamodel;

namespace Graphics {
class SceneListener;
class SceneManager;
class ResourceManager;
class MaterialManager;
class PostFXManager;

// VisualSystem Class:
// Provides an interface for the application's graphics.
// VisualSystem is in charge of the different rendering passes;
// pipeline provides a convenient interface for some functionality.
class VisualSystem {
  private:
    // Frame
    uint64_t frame;

    // Managers
    std::unique_ptr<Pipeline> pipeline;
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

    ResourceManager* getResourceManager() const;
    MaterialManager* getMaterialManager() const;
    SceneListener* getSceneListener() const;
    SceneManager* getSceneManager() const;
    RenderManager* getRenderManager() const;
    LightManager* getLightManager() const;
    Pipeline* getPipeline() const;
};
} // namespace Graphics
} // namespace Engine