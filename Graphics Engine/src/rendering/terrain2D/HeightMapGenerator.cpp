#include "HeightMapGenerator.h"

#include <cmath>

#include "rendering/core/Device.h"

#include "rendering/ImGui.h"
#include "rendering/resources/TextureBuilder.h"

namespace Engine
{
namespace Graphics
{
static constexpr int kHeightMapSampleCount = 256;
static constexpr int kHeightMapSliceCount = 256;

HeightMapGenerator::HeightMapGenerator(Device* device)
    : mNoise()
{
    mHeightmap = device->createTexture(
        "Terrain Heightmap", TextureLayout::R32_FLOAT,
        TextureUsage::ShaderResource, kHeightMapSampleCount,
        kHeightMapSampleCount, kHeightMapSliceCount);

    mFreeList.reserve(kHeightMapSliceCount);
    for (size_t i = 0; i < kHeightMapSliceCount; i++)
        mFreeList.emplace_back(kHeightMapSliceCount - i);
}
HeightMapGenerator::~HeightMapGenerator() = default;

HeightmapAllocation HeightMapGenerator::allocate()
{
    const HeightmapAllocation handle = mFreeList.back();
    mFreeList.pop_back();
    return handle;
}
void HeightMapGenerator::free(HeightmapAllocation handle)
{
    mFreeList.push_back(handle);
}

void HeightMapGenerator::generateHeightMap(Vector2 xzMin,
                                           Vector2 xzMax,
                                           DeviceContext* context)
{
    TextureBuilder builder(kHeightMapSampleCount, kHeightMapSampleCount,
                           TextureLayout::R32_FLOAT);

    TextureColor texel;
    auto& height = texel.asType<TextureColor::FloatR32>();

    const Vector2 extents = xzMax - xzMin;
    const float distBetweenSamplesInv = 1 / float(kHeightMapSampleCount - 1);

    for (int x = 0; x < kHeightMapSampleCount; x++)
    {
        for (int z = 0; z < kHeightMapSampleCount; z++)
        {
            const float worldX =
                xzMin.x + x * distBetweenSamplesInv * extents.x;
            const float worldZ =
                xzMin.y + z * distBetweenSamplesInv * extents.y;

            height.r = sampleHeight(worldX, worldZ);
            builder.setColor(x, z, texel);
        }
    }

    assert(mHeightmap);
    context->updateTexture(mHeightmap, 0, builder.getData().data(),
                           builder.getData().size());
}

void HeightMapGenerator::imGui()
{
#if defined(IMGUI_ENABLED)
    if (!ImGui::CollapsingHeader("Height Map Settings"))
        return;

    ImGui::SliderFloat("Noise Frequency", &frequency, 0.0f, 0.1f);
    ImGui::SliderInt("Noise Octaves", &octaves, 0, 10);
    ImGui::SliderFloat("Noise Persistence", &persistence, 0.0f, 2.f);

    ImGui::SliderFloat("Exponential", &exponential, 0.5f, 5.f);

    ImGui::SliderFloat("Height Minimum", &heightMin, -100.f, 25.f);
    ImGui::SliderFloat("Height Maximum", &heightMax, -25.f, 500.f);

    static bool viewHeightmap = false;
    ImGui::Checkbox("View Heightmap", &viewHeightmap);

    if (viewHeightmap)
    {
        mHeightmap->doImgui();
    }
#endif
}

void HeightMapGenerator::seed(uint32_t seed) { mNoise.seed(seed); }

float HeightMapGenerator::sampleHeight(float x, float z) const
{
    float noise = mNoise.octaveNoise2D(frequency * x, frequency * z, octaves,
                                       persistence);
    noise = pow(noise, exponential);
    return noise * (heightMax - heightMin) + heightMin;
}

} // namespace Graphics
} // namespace Engine