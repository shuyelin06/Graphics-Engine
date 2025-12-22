#pragma once

#include <cstdint>

#include "math/PerlinNoise.h"
#include "math/SDF.h"

namespace Engine {
using namespace Math;

namespace Datamodel {
// Unit of operation for the Terrain Generator.
struct TerrainSample {
    // Represents the distance from the nearest surface.
    // - if inside surface, + if outside. Surface is at 0.
    float surface_dist;

    // Intersect: Only include "ground" that both generating functions have
    TerrainSample operator&&(const TerrainSample& other);
    // Union: Inlcude all "ground" either generating function has
    TerrainSample operator||(const TerrainSample& other);
    // Negate: Swap "ground" and "air"
    TerrainSample operator!();
};

// TerrainGenerator:
// Responsible for generating the terrain data on the datamodel side.
class TerrainGenerator {
  private:
    uint32_t seed;

    // Generation Configs
    struct GenerationConfig {
        bool enable_height_field;
        bool enable_caves;
    } generation_config;

    PerlinNoise noise_height_field;
    struct HeightConfig {
        float max_height = 100.f;
        float min_height = -10.f;

        float elevation_dropoff = 1.f;
        float frequency = 0.0075f;
    } height_config;

    // Terrain Generation Config
    struct CaveConfig {
        float surface_blob_size = 0.5f;
        float frequency = 0.015f;
    } cave_config;

    // Perlin Noise Generators

    PerlinNoise noise_func;

  public:
    TerrainGenerator();

    // ImGui Display, Called by Terrain
    void propertyDisplay();

    void seedGenerator(unsigned int new_seed);
    float sampleTerrainGenerator(float x, float y, float z);

  private:
    // Terrain generating functions. These return samples that represent the
    // terrain's surface
    TerrainSample generateHeightField(float x, float y, float z);
    TerrainSample generateCaves(float x, float y, float z);
};

} // namespace Datamodel
} // namespace Engine