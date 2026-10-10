#pragma once

#include <memory>
#include <string_view>
#include <unordered_map>

#include "rendering/core/Device.h"
#include "rendering/core/Texture.h"

#include "rendering/resources/ResourceManager.h"
#include "rendering/postfx/PostFXManager.h"

namespace Engine
{
namespace Graphics
{
class TerrainMaterialStore
{
  private:
    Device* mDevice;
    ResourceManager* mResourceManager;
    PostFXManager* mPostFXManager;

    std::unordered_map<std::string_view, int> mMaterialIndexMap;

    std::shared_ptr<Texture> mAlbedoArray;
    std::vector<uint8_t> mFreeList;

  public:
    TerrainMaterialStore(Device* device,
                         ResourceManager* resourceManager,
                         PostFXManager* mPostFXManager);

    void initializeMaterials();
    int getMaterialIndex(std::string_view name);
    void streamMaterials(DeviceContext* context);

    std::shared_ptr<Texture>& getAlbedoArray() { return mAlbedoArray; }

  private:
    bool streamAlbedoTexture(DeviceContext* context,
                             std::string_view name,
                             std::string_view path);
};

} // namespace Graphics
} // namespace Engine