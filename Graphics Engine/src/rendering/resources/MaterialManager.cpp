#include "MaterialManager.h"

namespace Engine {
namespace Graphics {

class MaterialManagerImpl {
  public:
    MaterialManagerImpl();
    ~MaterialManagerImpl();
};

std::unique_ptr<MaterialManager> MaterialManager::create() {
    std::unique_ptr<MaterialManager> ptr =
        std::unique_ptr<MaterialManager>(new MaterialManager());
    ptr->mImpl = std::make_unique<MaterialManagerImpl>();
    return ptr;
}

MaterialManager::MaterialManager() = default;
MaterialManager::~MaterialManager() = default;

MaterialManagerImpl::MaterialManagerImpl() = default;
MaterialManagerImpl::~MaterialManagerImpl() = default;

} // namespace Graphics
} // namespace Engine