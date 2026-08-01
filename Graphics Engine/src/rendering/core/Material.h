#pragma once

#include <array>
#include <bitset>
#include <memory>

#include "Texture.h"

#include "RenderPass.h"
#include "RenderSettings.h"

#include "rendering/pipeline/EnumTypes.h"

namespace Engine
{
namespace Graphics
{
// A technique determines all shader bindings, including:
// - Vertex Shader + Resources
// - Pixel Shader + Resources
// A material is a collection of techniques by render pass.
struct ShaderResource
{
    enum class Type : uint8_t
    {
        Texture = 0,
        Unknown = 0xFF,
    };
    bool bound = false;
    Type resourceType = Type::Unknown;

    struct
    {
        std::shared_ptr<Texture> texture = nullptr;
        SamplerType sampleState{};
    } textureData;

    void initializeTextureResource(std::shared_ptr<Texture> texture,
                                   SamplerType sampleState);
};

struct Technique
{
    std::string vertexShader;
    std::array<ShaderResource, kVertexResourceMax> vResources;
    std::array<std::vector<uint8_t>, kVertexConstantBufferMax> vertexCBuffers;

    std::string pixelShader;
    std::array<ShaderResource, kPixelResourceMax> pResources;
    std::array<std::vector<uint8_t>, kPixelConstantBufferMax> pixelCbuffers;

    void clearVertexCB(uint8_t slot);
    void uploadVertexCBData(uint8_t slot, const void* src, size_t byteSize);
    void clearPixelCB(uint8_t slot);
    void uploadPixelCBData(uint8_t slot, const void* src, size_t byteSize);

    void bindVertexResource(uint8_t slot, const ShaderResource& resource);
    void bindPixelResource(uint8_t slot, const ShaderResource& resource);

    const ShaderResource& getVertexResource(uint8_t slot) const;
    const ShaderResource& getPixelResource(uint8_t slot) const;

    bool ready() const;
};

class Material
{
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