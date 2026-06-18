#pragma once

#include <memory>

#include "math/AABB.h"
#include "math/Matrix4.h"

#include "DrawCall.h"
#include "rendering/core/Material.h"
#include "rendering/core/Mesh.h"
#include "rendering/core/Texture.h"

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
// - Mesh: Vertex / Index Buffer (ResourceManager)
// - Material: All Shaders + Bindings (MaterialManager)
// - RenderPass: Other configurable pipeline fseatures (alpha blending,
//   rasterization, etc.)
// External systems can also set RenderView settings.

// A RenderView is an entity that the scene can be rendered from.
// Think of it like a "camera". RenderManager provides a few entrypoints for
// systems to provide it views
struct RenderView {
    Vector3 position;
    float zNear;
    Vector3 direction;
    float zFar;

    Matrix4 mWorldToLocal;
    Matrix4 mLocalToFrustum;
    Vector4 viewport; // x, y, width, height

    Texture* renderTarget = nullptr;
    Texture* depthStencil = nullptr;
};

// A DrawBlock is an abstract spatial entity that contains
// "renderable" things.
// Draw blocks can be instanced. If a draw block has the same mesh and material,
// but different instance data, then the render manager will automatically batch
// into one draw call.
// We cull and filter passes on draw blocks. Draw blocks then can be used
// to generate draw calls that the pipeline can read.
using DrawBlockKey = uint32_t;
inline constexpr DrawBlockKey kInvalidDrawBlockKey = 0xFFFF;

struct DrawBlock {
    AABB extents{};

    Mesh* mesh = nullptr;
    Material* material = nullptr;

    InstanceData* instanceData = nullptr;
    int numInstances = 1;

    DrawBlock();

    void initialize(AABB _extents, Mesh* mesh, Material* material);
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
    void updateInstanceData(const DrawBlockKey key, InstanceData instanceData, int numInstances = 1);
    void removeDrawBlock(const DrawBlockKey key);

    void setMainView(const RenderView& view);
    void setShadowViews(const RenderView* viewArr, uint32_t count);

    void perform();

  private:
    RenderManager();
    std::unique_ptr<RenderManagerImpl> mImpl;
};

} // namespace Graphics
} // namespace Engine