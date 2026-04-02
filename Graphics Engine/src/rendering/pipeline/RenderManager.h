#pragma once

#include "d3d11.h"

#include <memory>

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

enum PipelineRenderPass {
    kRenderPass_Terrain = 0,
    kRenderPass_Count_,
};

class PipelineRenderPassSet {
    static_assert(PipelineRenderPass::kRenderPass_Count_ <= sizeof(uint8_t));
    uint8_t bitMask;

  public:
    PipelineRenderPassSet(uint8_t bitMask);
    bool hasRenderPass(PipelineRenderPass pass) const;
};

class RenderMesh {
  public:
    virtual void bindAndExecute(Pipeline* pipeline,
                                ID3D11DeviceContext* context) = 0;
};

class RenderMaterial {
  public:
    virtual void bind(Pipeline* pipeline, ID3D11DeviceContext* context) = 0;
};

struct DrawCall {
    std::shared_ptr<RenderMesh> mesh;
    std::shared_ptr<RenderMaterial> material;
};

class RenderManager {
  public:
    static std::unique_ptr<RenderManager> create(VisualSystem* engine,
                                                 ID3D11DeviceContext* context,
                                                 ID3D11Device* device);
    ~RenderManager();

    void submitDrawCall(const PipelineRenderPassSet passes,
                        const DrawCall& drawCall);
    void perform();

  private:
    RenderManager();
    std::unique_ptr<RenderManagerImpl> mImpl;
};

} // namespace Graphics
} // namespace Engine