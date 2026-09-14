#include "TerrainMaterialStore.h"

#include "math/Compute.h"

namespace Engine
{
namespace Graphics
{
constexpr int kMaterialSliceTexelSize = 1024;
constexpr int kMaterialSliceCount = 4;

TerrainMaterialStore::TerrainMaterialStore(Device* device,
                                           ResourceManager* resourceManager)
    : mDevice(device)
    , mResourceManager(resourceManager)
    , mMaterialIndexMap()
    , mAlbedoArray()
{
}

int TerrainMaterialStore::getMaterialIndex(std::string_view name)
{
    auto iter = mMaterialIndexMap.find(name);
    if (iter != mMaterialIndexMap.end())
        return iter->second;
    else
        return -1;
}

void TerrainMaterialStore::initializeMaterials()
{
    mAlbedoArray = mDevice->createTexture(
        "Terrain Albedo Atlas", TextureLayout::R8G8B8A8_UNORM,
        TextureUsage::ShaderResource, kMaterialSliceTexelSize,
        kMaterialSliceTexelSize, kMaterialSliceCount,
        Math::computeMipCount(kMaterialSliceTexelSize,
                              kMaterialSliceTexelSize));
    for (int i = kMaterialSliceCount - 1; i >= 0; i--)
        mFreeList.push_back(i);
}

void TerrainMaterialStore::streamMaterials(DeviceContext* context)
{
    streamAlbedoTexture(context, "Grass", "terrain/Grass.png");
    streamAlbedoTexture(context, "Dirt", "terrain/Dirt.png");
    streamAlbedoTexture(context, "Cliff", "terrain/Cliff.png");
}

bool TerrainMaterialStore::streamAlbedoTexture(DeviceContext* context,
                                               std::string_view name,
                                               std::string_view path)
{
    // Material already exists, ignore load
    if (mMaterialIndexMap.contains(name))
        return true;
    // No free spots, fail
    if (mFreeList.empty())
        return false;

    // Allocate index
    const uint8_t freeIndex = mFreeList.back();
    mFreeList.pop_back();
    mMaterialIndexMap[name] = freeIndex;

    // Stream data into the allocated slice
    const bool success = mResourceManager->streamTextureData(
        context, path, mAlbedoArray, freeIndex);
    return success;
}

} // namespace Graphics
} // namespace Engine