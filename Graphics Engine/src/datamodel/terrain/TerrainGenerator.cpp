#include "TerrainGenerator.h"

#include <algorithm>
#include <assert.h>

#include "rendering/ImGui.h"

#include "TerrainConfig.h"

namespace Engine {
namespace Datamodel {
TerrainSample TerrainSample::operator&&(const TerrainSample& other) {
    TerrainSample output;
    output.surface_dist = std::max(surface_dist, other.surface_dist);
    return output;
}
TerrainSample TerrainSample::operator||(const TerrainSample& other) {
    TerrainSample output;
    output.surface_dist = std::min(surface_dist, other.surface_dist);
    return output;
}
TerrainSample TerrainSample::operator!() {
    TerrainSample output;
    output.surface_dist = -surface_dist;
    return output;
}

TerrainGenerator::TerrainGenerator() { seedGenerator(0); }

void TerrainGenerator::propertyDisplay() {
#ifdef IMGUI_ENABLED
    ImGui::SeparatorText("Terrain Generation Config");

    static int terrain_seed = 0;
    ImGui::SliderInt("Generation Seed", &terrain_seed, 0, 0xFFF);
    seed = terrain_seed;

    ImGui::Checkbox("Height Field Config",
                    &generation_config.enable_height_field);
    ImGui::Indent();
    {
        ImGui::SliderFloat("Min Height", &height_config.min_height, -100.f,
                           height_config.max_height);
        ImGui::SliderFloat("Max Height", &height_config.max_height,
                           height_config.min_height, 100.f);
        ImGui::SliderFloat("Elevation Dropoff", &height_config.elevation_dropoff,
                           0.01f, 5.0f);
        ImGui::SliderFloat("Height Noise Frequency", &height_config.frequency,
                           0.0f, 0.02f);
    }
    ImGui::Unindent();

    ImGui::Checkbox("Cave Config", &generation_config.enable_caves);
    ImGui::Indent();
    {
        ImGui::SliderFloat("Surface Blob size", &cave_config.surface_blob_size,
                           0.0f, 1.f);
        ImGui::SliderFloat("Cave Frequency", &cave_config.frequency, 0.0f, 0.04f);
    }
    ImGui::Unindent();
#endif
}

// Seed
void TerrainGenerator::seedGenerator(unsigned int new_seed) {
    if (seed != new_seed) {
        seed = new_seed;
        noise_func.seed(new_seed);
        // TODO maybe jitter the seed
        noise_height_field.seed(new_seed);
    }
}

// SampleSurfaceNoiseFunction:
// Deterministic function that, given a seed, samples a noise function to return
// a float where negatives are "inside the ground", positives are "in air", and
// 0 is where the surface is.
float TerrainGenerator::sampleTerrainGenerator(float x, float y,
                                               float z) const {
    TerrainSample sample;
    sample.surface_dist = FLT_MAX;

    const auto& config = generation_config;

    bool has_surface = false;

    /*
    
    if (config.enable_height_field) {
        sample = generateHeightField(x, y, z);
        has_surface = true;
    }
    */

    sample.surface_dist = SDFSphere(Vector3(x, y, z), 10);
    sample = sample || generateCaves(x, y, z);

    /*
    if (true || config.enable_caves) {
        if (has_surface) {
            sample = sample && generateCaves(x, y, z);
        } else {
            sample = generateCaves(x, y, z);
        }
    }
    
    */
    

    return sample.surface_dist;
}

TerrainSample TerrainGenerator::generateHeightField(float x, float y,
                                                    float z) const {
    TerrainSample sample;
    const auto& config = height_config;

    // Sample our height map
    float noise_val =
        noise_height_field.noise2D(x * config.frequency, z * config.frequency);
    noise_val = pow(noise_val, config.elevation_dropoff);

    // Convert to surface height SDF
    const float surface_height =
        noise_val * (config.max_height - config.min_height) + config.min_height;
    sample.surface_dist = y - surface_height;

    return sample;
}

TerrainSample TerrainGenerator::generateCaves(float x, float y, float z) const {
    TerrainSample sample;

    // https://accidentalnoise.sourceforge.net/minecraftworlds.html
    // Sample our noise function
    const float frequency = cave_config.frequency;
    const float inv_freq = 1 / frequency;
    float noise_val = noise_func.noise3D(frequency * x, frequency * y, frequency * z);

    // Offset sampled value as marching cubes considers a
    // surface where 0 is.
    float surface_dist = (noise_val - cave_config.surface_blob_size) * inv_freq;
    sample.surface_dist = surface_dist;
    // Flip value because this will be embedded in the height field
    return !sample;
}

} // namespace Datamodel
} // namespace Engine