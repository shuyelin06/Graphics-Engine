#include "MaterialManager.h"

#include <assert.h>
#include <unordered_map>

namespace Engine {
namespace Graphics {
using DefaultMaterialParams = MaterialManager::DefaultMaterialParams;
using TerrainMaterialParams = MaterialManager::TerrainMaterialParams;
class MaterialManagerImpl {
  public:
    MaterialManagerImpl();
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
const Technique* Material::getTechnique(const RenderPass pass) const {
    return techniques[pass];
}
bool Material::ready() const {
    bool ready = true;
    for (const Technique* technique : techniques) {
        ready = ready && technique->ready;
    }
    return ready;
}

std::unique_ptr<MaterialManager> MaterialManager::create() {
    std::unique_ptr<MaterialManager> ptr =
        std::unique_ptr<MaterialManager>(new MaterialManager());
    ptr->mImpl = std::make_unique<MaterialManagerImpl>();
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

MaterialManagerImpl::MaterialManagerImpl() = default;
MaterialManagerImpl::~MaterialManagerImpl() = default;

std::shared_ptr<Material>
MaterialManagerImpl::createMaterial(const DefaultMaterialParams& params) {
    // TODO: Hash on material params for deduplication
    std::shared_ptr<Material> material = std::make_shared<Material>();

    Technique* technique = material->setTechnique(RenderPass::kOpaque);
    technique->vertexShader = "TexturedMesh";
    technique->pixelShader = "TexturedMesh";

    technique->ready = true;

    return material;
}

std::shared_ptr<Material>
MaterialManagerImpl::createMaterial(const TerrainMaterialParams& params) {
    std::shared_ptr<Material> material = std::make_shared<Material>();

    Technique* technique = material->setTechnique(RenderPass::kOpaque);
    technique->vertexShader = "Terrain";
    technique->pixelShader = "Terrain";

    technique->ready = true;

    return material;
}

} // namespace Graphics
} // namespace Engine