#include "TerrainGeneration.h"

#include <algorithm>
#include <math.h>

#include "math/SDF.h"

namespace Engine {
namespace Graphics {
// Unit of operation for the Terrain Generator.
struct TerrainSample {
    // Represents the distance from the nearest surface.
    // - if inside surface, + if outside. Surface is at 0.
    float surface_dist;

    // Intersect: Only include "ground" that both generating functions have
    TerrainSample operator&&(const TerrainSample& other) {
        TerrainSample output;
        output.surface_dist = max(surface_dist, other.surface_dist);
        return output;
    }
    // Union: Inlcude all "ground" either generating function has
    TerrainSample operator||(const TerrainSample& other) {
        TerrainSample output;
        output.surface_dist = min(surface_dist, other.surface_dist);
        return output;
    }
    // Negate: Swap "ground" and "air"
    TerrainSample operator!() {
        TerrainSample output;
        output.surface_dist = -surface_dist;
        return output;
    }
};

class TerrainGenerationImpl {
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
    TerrainGenerationImpl();
    ~TerrainGenerationImpl();

    void seedGenerator(unsigned int new_seed);

    // TerrainOctree::SDFGeneratorDelegate Implementation
    float sampleSDF(float x, float y, float z) const;

  private:
    TerrainSample generateHeightField(float x, float y, float z) const;
    TerrainSample generateCaves(float x, float y, float z) const;
};

std::unique_ptr<TerrainGeneration> TerrainGeneration::create() {
    std::unique_ptr<TerrainGeneration> ptr =
        std::unique_ptr<TerrainGeneration>(new TerrainGeneration());
    ptr->mImpl = std::make_unique<TerrainGenerationImpl>();
    return std::move(ptr);
}

TerrainGeneration::TerrainGeneration() = default;
TerrainGeneration::~TerrainGeneration() = default;

void TerrainGeneration::seedGenerator(unsigned int new_seed) {
    mImpl->seedGenerator(new_seed);
}

float TerrainGeneration::sampleSDF(float x, float y, float z) const {
    return mImpl->sampleSDF(x, y, z);
}

TerrainGenerationImpl::TerrainGenerationImpl() { seedGenerator(0); }
TerrainGenerationImpl::~TerrainGenerationImpl() = default;

void TerrainGenerationImpl::seedGenerator(unsigned int new_seed) {
    if (seed != new_seed) {
        seed = new_seed;
        noise_func.seed(new_seed);
        // TODO maybe jitter the seed
        noise_height_field.seed(new_seed);
    }
}

// Deterministic function that, given a seed, samples a noise function to return
// a float where negatives are "inside the ground", positives are "in air", and
// 0 is where the surface is.
float TerrainGenerationImpl::sampleSDF(float x, float y, float z) const {
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

TerrainSample TerrainGenerationImpl::generateHeightField(float x, float y,
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

TerrainSample TerrainGenerationImpl::generateCaves(float x, float y,
                                                   float z) const {
    TerrainSample sample;

    // https://accidentalnoise.sourceforge.net/minecraftworlds.html
    // Sample our noise function
    const float frequency = cave_config.frequency;
    const float inv_freq = 1 / frequency;
    float noise_val =
        noise_func.noise3D(frequency * x, frequency * y, frequency * z);

    // Offset sampled value as marching cubes considers a
    // surface where 0 is.
    float surface_dist = (noise_val - cave_config.surface_blob_size) * inv_freq;
    sample.surface_dist = surface_dist;
    // Flip value because this will be embedded in the height field
    return !sample;
}

} // namespace Graphics
} // namespace Engine