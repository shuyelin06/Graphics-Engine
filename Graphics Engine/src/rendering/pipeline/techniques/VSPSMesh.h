#pragma once

#include "rendering/core/Material.h"
#include "rendering/core/Mesh.h"

#include "../RenderManager.h"
#include "rendering/lights/LightManager.h"
#include "rendering/resources/ResourceManager.h"

namespace Engine {
namespace Graphics {
class VSMesh : public VertexTechnique {
  private:
    std::shared_ptr<Mesh> mesh;
    Matrix4 worldFromLocal;

  public:
    VSMesh();

    void update(const std::shared_ptr<Mesh>& _mesh,
                const Matrix4& _localMatrix);

    // VertexTechnique Implementation
    void bindAndDraw(Pipeline* pipeline, PipelineRenderPass pass) override;
};

class PSMesh : public PixelTechnique {
  private:
    LightManager* mLightManager;
    ResourceManager* mResourceManager;

    Material mMaterial;

  public:
    PSMesh(LightManager* lightManager, ResourceManager* resourceManager);

    void update(Material material);

    void bind(Pipeline* pipeline) override;
};

} // namespace Graphics
} // namespace Engine