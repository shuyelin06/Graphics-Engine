#pragma once

#include "d3d11.h"

#include <memory>

#include "DrawCall.h"
#include "rendering/pipeline/PipelineManager.h"

namespace Engine {
namespace Graphics {
class VisualSystem;
class RenderManagerImpl;

// Rendering Architecture:
// The render manager supports a variety of render passes.
// Render passes are ran in a pre-determined order.
// To render, a system can add a draw call to a particular render pass.
// Any single draw call is configured as follows:
// - RenderMesh: Vertex Shader + Vertex / Index Buffer, and any other resources
// - RenderMaterial: Pixel Shader + any other resources
// - RenderPass: Other configurable pipeline features (alpha blending,
// rasterization, etc.)

// TODO:
// - Add batching to draw calls for instancing.
// - Shadows. Skinned Meshes. Normal Meshes
// - RenderManager should own pipeline.
// - Instancing support
class RenderManager {
  public:
    static std::unique_ptr<RenderManager> create(VisualSystem* engine,
                                                 ID3D11DeviceContext* context,
                                                 ID3D11Device* device);
    ~RenderManager();

    void submitDrawCall(const DrawCall& drawCall, RenderPassSet renderPasses);

    void perform();

  private:
    RenderManager();
    std::unique_ptr<RenderManagerImpl> mImpl;
};

} // namespace Graphics
} // namespace Engine