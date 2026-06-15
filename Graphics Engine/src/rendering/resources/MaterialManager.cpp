#include "MaterialManager.h"

#include <assert.h>
#include <unordered_map>

namespace Engine {
namespace Graphics {
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
void Technique::uploadPixelCBData(uint8_t slot, const void* src,
                                  size_t byteSize) {
    assert(slot <= kPixelConstantBufferMax);
    auto& cb = vertexCBuffers[slot];
    const size_t end = cb.size();
    cb.resize(end + byteSize);
    memcpy(cb.data() + end, src, byteSize);
}
void Technique::bindVertexShaderResource(uint8_t slot,
                                         std::shared_ptr<Texture> texture,
                                         TextureSampler sampleState) {
    assert(slot <= kVertexResourceMax);
    vResources[slot].texture = texture;
    vResources[slot].sampleState = sampleState;
    vResourcesFlag.set(slot);
}
bool Technique::hasVertexResource(uint8_t slot) const {
    assert(slot <= kVertexResourceMax);
    return vResourcesFlag.test(slot);
}
const Technique::BoundTexture&
Technique::getVertexResource(uint8_t slot) const {
    assert(slot <= kVertexResourceMax && vResourcesFlag.test(slot));
    return vResources[slot];
}

bool Technique::ready() const {
    bool ready = true;
    for (int slot = 0; slot < kVertexResourceMax; slot++) {
        if (hasVertexResource(slot)) {
            ready = ready && vResources[slot].texture->ready;
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

    return material;
}

} // namespace Graphics
} // namespace Engine