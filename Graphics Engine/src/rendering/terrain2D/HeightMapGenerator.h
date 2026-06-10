#pragma once

#include <stdint.h>

#include "math/PerlinNoise.h"
#include "math/Vector2.h"

namespace Engine {
using namespace Math;
namespace Graphics {
class HeightMapGenerator {
  private:
    PerlinNoise mNoise;

    float frequency = 0.005f;
    int octaves = 1;
    float persistence = 0.75f;

    float exponential = 3.5f;

    float heightMin = -30.f;
    float heightMax = 500.f;

  public:
    HeightMapGenerator();
    ~HeightMapGenerator();

    void imGui();

    void seed(uint32_t seed);

    float sampleHeight(const Vector2& xz) const;
    float sampleHeight(float x, float z) const;
};

} // namespace Graphics
} // namespace Engine