#include "HeightMapGenerator.h"

#include <cmath>

#include "rendering/ImGui.h"

namespace Engine {
namespace Graphics {
HeightMapGenerator::HeightMapGenerator() : mNoise() {}
HeightMapGenerator::~HeightMapGenerator() = default;

void HeightMapGenerator::imGui() {
#if defined(IMGUI_ENABLED)
    ImGui::SliderFloat("Noise Frequency", &frequency, 0.0f, 0.1f);
    ImGui::SliderInt("Noise Octaves", &octaves, 0, 10);
    ImGui::SliderFloat("Noise Persistence", &persistence, 0.0f, 2.f);

    ImGui::SliderFloat("Exponential", &exponential, 0.5f, 5.f);

    ImGui::SliderFloat("Heigh Minimum", &heightMin, -100.f, 25.f);
    ImGui::SliderFloat("Height Maximum", &heightMax, -25.f, 500.f);
#endif
}

void HeightMapGenerator::seed(uint32_t seed) { mNoise.seed(seed); }

float HeightMapGenerator::sampleHeight(const Vector2& xz) const {
    return sampleHeight(xz.x, xz.y);
}
float HeightMapGenerator::sampleHeight(float x, float z) const {
    float noise = mNoise.octaveNoise2D(frequency * x, frequency * z, octaves, persistence);
    noise = pow(noise, exponential);
    return noise * (heightMax - heightMin) + heightMin;
}

} // namespace Graphics
} // namespace Engine