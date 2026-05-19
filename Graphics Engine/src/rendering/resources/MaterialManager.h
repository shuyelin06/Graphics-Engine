#pragma once

#include <array>
#include <memory>
#include <string>
#include <vector>

#include "rendering/pipeline/RenderPass.h"

namespace Engine {
namespace Graphics {
class MaterialManagerImpl;

// A technique determines all shader bindings, including:
// - Vertex Shader + Resources
// - Pixel Shader + Resources
// A material is a collection of techniques by render pass.
constexpr uint8_t kVertexConstantBufferMax = 4;
constexpr uint8_t kPixelConstantBufferMax = 4;
struct Technique {
    std::string vertexShader;
    std::vector<uint8_t> vertexCBuffers[kVertexConstantBufferMax];

    std::string pixelShader;
    std::vector<uint8_t> pixelCbuffers[kPixelConstantBufferMax];

    std::atomic<bool> ready;
};

class Material {
  private:
    std::array<Technique*, RenderPass::_Count_> techniques;

  public:
    Material();
    ~Material();

    // Used by Material Manager
    void setTechnique(const RenderPass pass, Technique* technique);

    // Used by Render Manager
    bool hasTechnique(const RenderPass pass) const;
    const Technique* getTechnique(const RenderPass pass) const;

    // Used by Other Systems
    bool ready() const;
};

// MaterialManager Class:
// Manages materials.
// In this engine, a "material" is essentially a configurable PixelTechnique
// that applies to a specific pass.
class MaterialManager {
  public:
    struct DefaultMaterialParams {
        std::string colormap;
    };
    struct TerrainMaterialParams {};

    static std::unique_ptr<MaterialManager> create();
    ~MaterialManager();

    std::shared_ptr<Material>
    createMaterial(const DefaultMaterialParams& params);
    std::shared_ptr<Material>
    createMaterial(const TerrainMaterialParams& params);

  private:
    std::unique_ptr<MaterialManagerImpl> mImpl;
    MaterialManager();
};

} // namespace Graphics
} // namespace Engine