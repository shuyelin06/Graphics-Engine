#pragma once
#pragma once

#include "d3d11.h"

#include <memory>

#include "math/AABB.h"

#include "DrawCall.h"
#include "rendering/pipeline/PipelineManager.h"

namespace Engine {
using namespace Math;

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

// A DrawBlock is an abstract spatial entity that contains
// "renderable" things.
// We cull and filter passes on draw blocks. Draw blocks then can be used
// to generate draw calls that the pipeline can read.
using DrawBlockKey = uint32_t;
constexpr DrawBlockKey kInvalidDrawBlockKey = 0xFFFF;
struct DrawBlock {
    AABB extents{};

    RenderPassSet supportedPasses;
    // Draw Call that will be submitted if Draw Block is rendered
    // TODO: Draw Blocks should prob support multiple draw calls.
    DrawCall drawCall;

    DrawBlock();

    void initialize(AABB _extents, RenderPassSet _supportedPasses,
                    DrawCall _drawCall);
};

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

    DrawBlockKey addDrawBlock(const DrawBlock& block);
    void removeDrawBlock(const DrawBlockKey key);

    void perform();

  private:
    RenderManager();
    std::unique_ptr<RenderManagerImpl> mImpl;
};

} // namespace Graphics
} // namespace Engine