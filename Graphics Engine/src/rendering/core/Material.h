#pragma once

#include <array>
#include <memory>

#include "RenderPass.h"
#include "RenderSettings.h"

namespace Engine {
namespace Graphics {
// A technique determines all shader bindings, including:
// - Vertex Shader + Resources
// - Pixel Shader + Resources
// A material is a collection of techniques by render pass.
struct Technique {
    std::string vertexShader;
    std::array<std::vector<uint8_t>, kVertexConstantBufferMax> vertexCBuffers;

    std::string pixelShader;
    std::array<std::vector<uint8_t>, kPixelConstantBufferMax> pixelCbuffers;

    std::atomic<bool> ready;

    void uploadVertexCBData(uint8_t slot, const void* src, size_t byteSize);
    void uploadPixelCBData(uint8_t slot, const void* src, size_t byteSize);
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
    const Technique* getTechnique(const RenderPass pass) const;

    // Used by Other Systems
    bool ready() const;
};

} // namespace Graphics
} // namespace Engine