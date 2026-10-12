#include "TerrainMaterialStore.h"

#include "math/Compute.h"

namespace Engine
{
namespace Graphics
{
constexpr int kMaterialSliceTexelSize = 1024;
constexpr int kMaterialSliceCount = 4;

TerrainMaterialStore::TerrainMaterialStore(Device* device,
                                           ResourceManager* resourceManager,
                                           PostFXManager* postFXManager)
    : mDevice(device)
    , mResourceManager(resourceManager)
    , mPostFXManager(postFXManager)
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
    const uint8_t mipCount =
        Math::computeMipCount(kMaterialSliceTexelSize, kMaterialSliceTexelSize);

    mAlbedoArray = mDevice->createTexture(
        "Terrain Albedo Array", TextureLayout::R8G8B8A8_UNORM,
        TextureUsage::ShaderResource | TextureUsage::RenderTarget,
        kMaterialSliceTexelSize, kMaterialSliceTexelSize, kMaterialSliceCount,
        mipCount);
    mDisplacementArray = mDevice->createTexture(
        "Terrain Displacement Array", TextureLayout::R32_FLOAT,
        TextureUsage::ShaderResource | TextureUsage::RenderTarget,
        kMaterialSliceTexelSize, kMaterialSliceTexelSize, kMaterialSliceCount,
        mipCount);

    for (int i = kMaterialSliceCount - 1; i >= 0; i--)
        mFreeList.push_back(i);
}

void TerrainMaterialStore::streamMaterials(DeviceContext* context)
{
    streamMaterial(context, "Grass", "terrain/Grass/");
    streamMaterial(context, "Dirt", "terrain/Dirt/");
}

bool TerrainMaterialStore::streamMaterial(DeviceContext* context,
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

    auto textureBlit = [this, context](std::shared_ptr<Texture>& targetArray,
                                       uint8_t slice,
                                       const std::shared_ptr<Texture>& source) {
        std::shared_ptr<RenderTarget> target = mDevice->createRenderTarget(
            targetArray, RenderTargetType::Color, slice);
        PostFXParams params(target);
        context->bindRenderTargets(target, nullptr,
                                   DepthSettings::Depth_Disabled,
                                   BlendSettings::Blend_Default);

        context->bindPixelTexture(0, source, SamplerSettings::Point);
        mPostFXManager->renderScreenspacePass(context, params, "PassthroughPS");

        context->generateMips(targetArray);
    };

    const std::string albedoPath = std::string(path) + "Albedo.png";
    const std::string displacementPath = std::string(path) + "Displacement.png";

    const std::shared_ptr<Texture> albedo =
        mResourceManager->loadTextureFromFile(albedoPath.c_str(),
                                              TextureResourceType::kAlbedoMap);
    const std::shared_ptr<Texture> displacement =
        mResourceManager->loadTextureFromFile(
            displacementPath.c_str(), TextureResourceType::kDisplacementMap);

    textureBlit(mAlbedoArray, freeIndex, albedo);
    textureBlit(mDisplacementArray, freeIndex, displacement);

    return true;
}

void TerrainMaterialStore::imGui()
{
#if defined(IMGUI_ENABLED)
    if (!ImGui::CollapsingHeader("Terrain Materials"))
        return;

    static int selectedMaterialIndex = 0;
    ImGui::SliderInt("Material Index", &selectedMaterialIndex, 0,
                     kMaterialSliceCount);
    mAlbedoArray->renderToImGui(selectedMaterialIndex);
    mDisplacementArray->renderToImGui(selectedMaterialIndex);
#endif
}

} // namespace Graphics
} // namespace Engine