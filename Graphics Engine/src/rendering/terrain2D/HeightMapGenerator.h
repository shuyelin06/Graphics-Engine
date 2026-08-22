#pragma once

#include <memory>
#include <stdint.h>

#include "math/PerlinNoise.h"
#include "math/Vector2.h"

#include "rendering/core/Texture.h"

namespace Engine
{
using namespace Math;
namespace Graphics
{
class Device;
class DeviceContext;

using HeightmapAllocation = uint16_t;
class HeightMapGenerator
{
  private:
    // 2D Texture Array of heights. Each slice corresponds to a single node in the Terrain QuadTree.
    std::shared_ptr<Texture> mHeightmap = nullptr;
    // Free-list of array slices we can allocate from
    std::vector<HeightmapAllocation> mFreeList;

    PerlinNoise mNoise;

    float frequency = 0.005f;
    int octaves = 1;
    float persistence = 0.75f;

    float exponential = 3.5f;

    float heightMin = -30.f;
    float heightMax = 500.f;

  public:
    HeightMapGenerator(Device* device);
    ~HeightMapGenerator();

    const std::shared_ptr<Texture>& getTexture() const { return mHeightmap; }

    void seed(uint32_t seed);

    HeightmapAllocation allocate();
    void free(HeightmapAllocation handle);

    void
    generateHeightMap(Vector2 xzMin, Vector2 xzMax, DeviceContext* context);

    void imGui();

  private:
    float sampleHeight(float x, float z) const;
};

} // namespace Graphics
} // namespace Engine