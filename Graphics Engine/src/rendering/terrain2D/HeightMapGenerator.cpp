#include "HeightMapGenerator.h"

namespace Engine {
namespace Graphics {
const float kFrequency = 0.15f;
constexpr float kHeightMinimum = 0.f;
constexpr float kHeightMaximum = 30.f;

HeightMapGenerator::HeightMapGenerator() : mNoise() {}
HeightMapGenerator::~HeightMapGenerator() = default;

void HeightMapGenerator::seed(uint32_t seed) { mNoise.seed(seed); }

float HeightMapGenerator::sampleHeight(const Vector2& xz) const {
    return sampleHeight(xz.x, xz.y);
}
float HeightMapGenerator::sampleHeight(float x, float z) const {
    const float noise = mNoise.noise2D(kFrequency * x, kFrequency * z);
    return noise * (kHeightMaximum - kHeightMinimum) + kHeightMinimum;
}

} // namespace Graphics
} // namespace Engine