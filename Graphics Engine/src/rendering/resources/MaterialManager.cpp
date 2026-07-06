#include "MaterialManager.h"

#include <assert.h>
#include <unordered_map>

namespace Engine {
namespace Graphics {
void ShaderResource::initializeTextureResource(std::shared_ptr<Texture> texture,
                                               SamplerType sampleState) {
    bound = true;
    resourceType = Type::Texture;
    textureData.texture = texture;
    textureData.sampleState = sampleState;
}

void Technique::clearVertexCB(uint8_t slot) {
    assert(slot <= kVertexConstantBufferMax);
    auto& cb = vertexCBuffers[slot];
    cb.clear();
}
void Technique::uploadVertexCBData(uint8_t slot, const void* src,
                                   size_t byteSize) {
    assert(slot <= kVertexConstantBufferMax);
    auto& cb = vertexCBuffers[slot];
    const size_t end = cb.size();
    cb.resize(end + byteSize);
    memcpy(cb.data() + end, src, byteSize);
}
void Technique::clearPixelCB(uint8_t slot) {
    assert(slot <= kPixelConstantBufferMax);
    auto& cb = pixelCbuffers[slot];
    cb.clear();
}
void Technique::uploadPixelCBData(uint8_t slot, const void* src,
                                  size_t byteSize) {
    assert(slot <= kPixelConstantBufferMax);
    auto& cb = pixelCbuffers[slot];
    const size_t end = cb.size();
    cb.resize(end + byteSize);
    memcpy(cb.data() + end, src, byteSize);
}
void Technique::bindVertexResource(uint8_t slot,
                                   const ShaderResource& resource) {
    assert(slot <= kVertexResourceMax);
    vResources[slot] = resource;
}
void Technique::bindPixelResource(uint8_t slot,
                                  const ShaderResource& resource) {
    assert(slot <= kPixelResourceMax);
    pResources[slot] = resource;
}
const ShaderResource& Technique::getVertexResource(uint8_t slot) const {
    assert(slot <= kVertexResourceMax);
    return vResources[slot];
}
const ShaderResource& Technique::getPixelResource(uint8_t slot) const {
    assert(slot <= kPixelResourceMax);
    return pResources[slot];
}

bool Technique::ready() const {
    bool ready = true;

    for (int slot = 0; slot < kVertexResourceMax; slot++) {
        auto& resource = vResources[slot];
        if (resource.bound) {
            if (resource.resourceType == ShaderResource::Type::Texture) {
                ready = ready && resource.textureData.texture->ready;
            }
        }
    }

    for (int slot = 0; slot < kPixelResourceMax; slot++) {
        auto& resource = pResources[slot];
        if (resource.bound) {
            if (resource.resourceType == ShaderResource::Type::Texture) {
                ready = ready && resource.textureData.texture->ready;
            }
        }
    }

    return ready;
}
using DefaultMaterialParams = MaterialManager::DefaultMaterialParams;
using TerrainMaterialParams = MaterialManager::TerrainMaterialParams;
class MaterialManagerImpl {
  private:
    ResourceManager* resourceManager;

    std::unordered_map<uint32_t, std::weak_ptr<Material>> materialMap;

  public:
    MaterialManagerImpl(ResourceManager* resourceManager);
    ~MaterialManagerImpl();

    std::shared_ptr<Material>
    createMaterial(const DefaultMaterialParams& params);
    std::shared_ptr<Material>
    createMaterial(const TerrainMaterialParams& params);
};

Material::Material() : techniques(nullptr) {}
Material::~Material() {
    for (Technique* technique : techniques) {
        if (technique != nullptr)
            delete technique;
    }
}

Technique* Material::setTechnique(const RenderPass pass) {
    assert(techniques[pass] == nullptr);
    techniques[pass] = new Technique();
    return techniques[pass];
}

bool Material::hasTechnique(const RenderPass pass) const {
    return techniques[pass] != nullptr;
}
Technique* Material::getTechnique(const RenderPass pass) const {
    return techniques[pass];
}
bool Material::ready() const {
    bool ready = true;
    for (const Technique* technique : techniques) {
        if (technique)
            ready = ready && technique->ready();
    }
    return ready;
}

MD5Hash MaterialManager::DefaultMaterialParams::generateHash() const {
    return hashMD5(colormap.data(), colormap.size());
}

std::unique_ptr<MaterialManager>
MaterialManager::create(ResourceManager* resourceManager) {
    std::unique_ptr<MaterialManager> ptr =
        std::unique_ptr<MaterialManager>(new MaterialManager());
    ptr->mImpl = std::make_unique<MaterialManagerImpl>(resourceManager);
    return ptr;
}

MaterialManager::MaterialManager() = default;
MaterialManager::~MaterialManager() = default;

std::shared_ptr<Material>
MaterialManager::createMaterial(const DefaultMaterialParams& params) {
    return mImpl->createMaterial(params);
}
std::shared_ptr<Material>
MaterialManager::createMaterial(const TerrainMaterialParams& params) {
    return mImpl->createMaterial(params);
}

MaterialManagerImpl::MaterialManagerImpl(ResourceManager* resourceManager)
    : resourceManager(resourceManager) {}
MaterialManagerImpl::~MaterialManagerImpl() = default;

std::shared_ptr<Material>
MaterialManagerImpl::createMaterial(const DefaultMaterialParams& params) {
    MD5Hash md5 = params.generateHash();
    // TODO figure out better way to combine the hash values
    const uint32_t hash = md5[0] ^ md5[1] ^ md5[2] ^ md5[3];

    if (auto iter = materialMap.find(hash); iter != materialMap.end()) {
        std::weak_ptr<Material> materialWeak = materialMap[hash];
        std::shared_ptr<Material> material = materialWeak.lock();

        if (material) {
            return material;
        } else {
            materialMap.erase(iter);
        }
    }

    std::shared_ptr<Material> material = std::make_shared<Material>();

    Technique* technique = material->setTechnique(RenderPass::kOpaque);
    technique->vertexShader = "TexturedMesh";
    technique->pixelShader = "TexturedMesh";

    materialMap[hash] = material;

    return material;
}

std::shared_ptr<Material>
MaterialManagerImpl::createMaterial(const TerrainMaterialParams& params) {
    std::shared_ptr<Material> material = std::make_shared<Material>();

    Technique* technique = material->setTechnique(RenderPass::kOpaque);
    technique->vertexShader = "Terrain";
    technique->pixelShader = "Terrain";

    TextureRequestParams texRequest("Terrain Colormap");
    texRequest.initCreateFromFile(params.colormap, false);

    ShaderResource colormap{};
    colormap.initializeTextureResource(
        resourceManager->requestTexture(texRequest),
        SamplerType::Sampler_Point);
    technique->bindPixelResource(4, colormap);

    return material;
}

} // namespace Graphics
} // namespace Engine