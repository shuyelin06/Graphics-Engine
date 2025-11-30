#include "TerrainGenerator.h"

#include <algorithm>S

#include "rendering/ImGui.h"

#include "TerrainConfig.h"

namespace Engine {
namespace Datamodel {
TerrainGenerator::TerrainGenerator() { seedGenerator(0); }

void TerrainGenerator::propertyDisplay() {
#ifdef IMGUI_ENABLED
    ImGui::SeparatorText("Terrain Generation Config");
    static int terrain_seed = 0;
    ImGui::SliderInt("Generation Seed", &terrain_seed, 0, 0xFFF);
    seed = terrain_seed;

    ImGui::SliderFloat("Surface Blob size",
                       &generation_config.surface_blob_size, 0.0f, 1.f);
    ImGui::SliderFloat("Frequency", &generation_config.frequency, 0.0f, 0.02f);
    ImGui::SliderInt("Octaves", &generation_config.num_octaves, 1, 4);
    ImGui::SliderFloat("Persistence", &generation_config.persistence, 0.0f,
                       1.f);
#endif
}

// Seed
void TerrainGenerator::seedGenerator(unsigned int new_seed) {
    if (seed != new_seed) {
        seed = new_seed;
        noise_func.seed(new_seed);
    }
}

// SampleSurfaceNoiseFunction:
// Deterministic function that, given a seed, samples a noise function to return
// a float where negatives are "inside the ground", positives are "in air", and
// 0 is where the surface is.
float TerrainGenerator::sampleTerrainGenerator(float x, float y, float z) {
    // https://accidentalnoise.sourceforge.net/minecraftworlds.html
    //
    // Sample our noise function
    const float frequency = generation_config.frequency;
    float val = noise_func.octaveNoise3D(
        frequency * x, frequency * y, frequency * z,
        generation_config.num_octaves, generation_config.persistence);

    // Fade our noise to 0 depending on how close we are to the
    // water line
    constexpr float FADE_RATE = 0.0075f;
    if (y >= TERRAIN_FADE_LINE) {
        val += (y - TERRAIN_FADE_LINE) * FADE_RATE;
    }
    // val = Clamp(val, 0.0f, 1.0f);

    // Offset sampled value as marching cubes considers a
    // surface where 0 is.
    return val - generation_config.surface_blob_size;
}

TerrainSample TerrainGenerator::intersectSamples(const TerrainSample& s1,
                                                 const TerrainSample& s2) {
    TerrainSample output;
    output.surface_dist = std::max(s1.surface_dist, s2.surface_dist);
    return output;
}
TerrainSample TerrainGenerator::unionSamples(const TerrainSample& s1,
                                             const TerrainSample& s2) {
    TerrainSample output;
    output.surface_dist = std::min(s1.surface_dist, s2.surface_dist);
    return output;
}

} // namespace Datamodel
} // namespace Engine