#include "Terrain.h"

#include <assert.h>

#include <map>

#include "rendering/VisualTerrain.h"

#include "math/Compute.h"
#include "math/Matrix3.h"
#include "math/PerlinNoise.h"
#include "math/Triangle.h"

#if defined(TERRAIN_DEBUG)
#include "rendering/VisualDebug.h"
#endif

constexpr float DISTANCE_BETWEEN_SAMPLES =
    HEIGHT_MAP_XZ_SIZE / (HEIGHT_MAP_XZ_SAMPLES - 1);

namespace Engine {
namespace Datamodel {
TerrainChunk::TerrainChunk(float _world_x, float _world_z) {
    world_x = _world_x;
    world_z = _world_z;

    visual_terrain = nullptr;

    // Sample the Perlin Noise to generate my height map
    for (int x = 0; x < HEIGHT_MAP_XZ_SAMPLES; x++) {
        for (int z = 0; z < HEIGHT_MAP_XZ_SAMPLES; z++) {
            reloadHeightMap(x, z);
        }
    }

    // Randomly create trees in the chunk
    int num_trees = Random(3, 10);
    for (int i = 0; i < num_trees; i++) {
        const float x = Random(0.f, HEIGHT_MAP_XZ_SIZE);
        const float z = Random(0.f, HEIGHT_MAP_XZ_SIZE);
        tree_locations.push_back(Vector2(x, z));
    }
}
TerrainChunk::~TerrainChunk() {
    if (visual_terrain != nullptr)
        visual_terrain->destroy();
}

// Get
// Returns data about the terrain.
float TerrainChunk::getX() const { return world_x; }
float TerrainChunk::getZ() const { return world_z; }

const std::vector<Vector2>& TerrainChunk::getTreeLocations() const {
    return tree_locations;
}

// ReloadTerrainChunk:
// Reloads a sample of the height-map by index, by calculating the x,z
// coordinates of that sample.
void TerrainChunk::reloadHeightMap(UINT index_x, UINT index_z) {
    // For our sample index, calculate the x,z coordinates of where we want to
    // sample.
    const float x = world_x + DISTANCE_BETWEEN_SAMPLES * index_x;
    const float z = world_z + DISTANCE_BETWEEN_SAMPLES * index_z;

    // Tuning the inputs to the noise, so that the terrain comes out smoothly.
    constexpr float ROUGHNESS = 0.0035f;
    constexpr float OFFSET = 1000.f;

    static PerlinNoise noise_func = PerlinNoise(30);
    const float noise = noise_func.octaveNoise2D(x * ROUGHNESS + OFFSET,
                                                 z * ROUGHNESS + OFFSET, 4, 6);
    const float height = noise * HEIGHT_MAP_Y_HEIGHT;

    height_map[index_x][index_z] = height;
}

// SampleTerrainHeight:
// Samples the height-map at a specific x,z value. Returns -1 if the x,z
// coordinates are outside of the terrain.
float TerrainChunk::sampleTerrainHeight(float x, float z) const {
    // Returns a floating point value telling us what height map samples our
    // coordinates are between
    const float index_x = (x - world_x) / DISTANCE_BETWEEN_SAMPLES;
    const float index_z = (z - world_z) / DISTANCE_BETWEEN_SAMPLES;

    // Check if the computed index is outside of our 2D heightmap
    if (index_x < 0 || HEIGHT_MAP_XZ_SAMPLES <= index_x)
        return -1.f;
    if (index_z < 0 || HEIGHT_MAP_XZ_SAMPLES <= index_z)
        return -1.f;

    // Bilinearly interpolate the height data.
    // To do this, find the x,z coordinates we are inside of our grid cell, x,z
    // in [0,1].
    const int x0 = floor(index_x);
    const int x1 = floor(index_x + 1);
    const int z0 = floor(index_z);
    const int z1 = floor(index_z + 1);

    const float height_00 = height_map[x0][z0];
    const float height_10 = height_map[x1][z0];
    const float height_01 = height_map[x0][z1];
    const float height_11 = height_map[x1][z1];

    const float x_dist = index_x - x0;
    const float z_dist = index_z - z0;

    /*const float height =
        CubicInterp(CubicInterp(height_00, height_10, x_dist),
                    CubicInterp(height_01, height_11, x_dist), z_dist);*/
    const float height = Lerp(Lerp(height_00, height_10, x_dist),
                              Lerp(height_01, height_11, x_dist), z_dist);

    return height;
}

// Bind___:
// Bind components to the terrain
void TerrainChunk::bindVisualTerrain(Graphics::VisualTerrain* _visual_terrain) {
    visual_terrain = _visual_terrain;
}
bool TerrainChunk::hasVisualTerrain() const {
    return visual_terrain != nullptr;
}

} // namespace Datamodel
} // namespace Engine