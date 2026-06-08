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

  public:
    HeightMapGenerator();
    ~HeightMapGenerator();

    void seed(uint32_t seed);

    float sampleHeight(const Vector2& xz) const;
    float sampleHeight(float x, float z) const;
};

} // namespace Graphics
} // namespace Engine