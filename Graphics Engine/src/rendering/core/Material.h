#pragma once

#include <array>
#include <bitset>
#include <memory>

#include "Texture.h"

#include "RenderPass.h"
#include "RenderSettings.h"

#include "rendering/pipeline/EnumTypes.h"

namespace Engine {
namespace Graphics {
// A technique determines all shader bindings, including:
// - Vertex Shader + Resources
// - Pixel Shader + Resources
// A material is a collection of techniques by render pass.
struct Technique {
    struct BoundTexture
    {
        std::shared_ptr<Texture> texture = nullptr;
        SamplerType sampleState;
    };

    std::string vertexShader;
    
    std::array<BoundTexture, kVertexResourceMax> vResources;
    std::bitset<kVertexResourceMax> vResourcesFlag;

    std::array<std::vector<uint8_t>, kVertexConstantBufferMax> vertexCBuffers;
    std::bitset<kVertexResourceMax> vertexResourcesFlag;

    std::string pixelShader;
    std::array<std::vector<uint8_t>, kPixelConstantBufferMax> pixelCbuffers;

    void clearVertexCB(uint8_t slot);
    void uploadVertexCBData(uint8_t slot, const void* src, size_t byteSize);
    void uploadPixelCBData(uint8_t slot, const void* src, size_t byteSize);

    void bindVertexShaderResource(uint8_t slot,
                                  std::shared_ptr<Texture> texture,
                                  SamplerType sampleState);
    
    bool hasVertexResource(uint8_t slot) const;
    const BoundTexture& getVertexResource(uint8_t slot) const;

    bool ready() const;
};

class Material {
  private:
    std::array<Technique*, RenderPass::_Count_> techniques;

  public:
    Material();
    ~Material();

    // Used by Material Manager
    Technique* setTechnique(const RenderPass pass);

    // Used by Render Manager
    bool hasTechnique(const RenderPass pass) const;
    Technique* getTechnique(const RenderPass pass) const;

    // Used by Other Systems
    bool ready() const;
};

} // namespace Graphics
} // namespace Engine