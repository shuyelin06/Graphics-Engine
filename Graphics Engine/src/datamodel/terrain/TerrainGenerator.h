#pragma once

#include "math/PerlinNoise.h"

namespace Engine {
using namespace Math;

namespace Datamodel {
// Unit of operation for the Terrain Generator.
struct TerrainSample {
    // Represents the distance from the nearest surface.
    // - if inside surface, + if outside. Surface is at 0.
    float surface_dist;
};

// TerrainGenerator:
// Responsible for generating the terrain data on the datamodel side.
class TerrainGenerator {
  private:
    unsigned int seed;

    // Terrain Generation Config
    struct TerrainGenerationConfig {
        float surface_blob_size = 0.5f;
        float frequency = 0.0075f;
        int num_octaves = 1;
        float persistence = 0.5f;
    } generation_config;

    // Perlin Noise Generator
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
    // TODO

    // Terrain sample operations. These let us combine multiple terrain
    // generation functions together.
    TerrainSample intersectSamples(const TerrainSample& s1,
                                   const TerrainSample& s2);
    TerrainSample unionSamples(const TerrainSample& s1,
                               const TerrainSample& s2);
};

} // namespace Datamodel
} // namespace Engine