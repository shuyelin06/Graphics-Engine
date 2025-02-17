#include "Terrain.h"

#include <assert.h>

#include <map>

#include "math/Compute.h"
#include "math/Matrix3.h"
#include "math/PerlinNoise.h"
#include "math/Triangle.h"

#if defined(TERRAIN_DEBUG)
#include "rendering/VisualDebug.h"
#endif

namespace Engine {
namespace Datamodel {
Terrain::Terrain(float _world_x, float _world_z) {
    world_x = _world_x;
    world_z = _world_z;

    for (int x = 0; x < HEIGHT_MAP_XZ_SAMPLES; x++) {
        for (int z = 0; z < HEIGHT_MAP_XZ_SAMPLES; z++) {
            reloadTerrainChunk(x, z);
        }
    }
}

// Get
// Returns data about the terrain.
float Terrain::getTerrainHeight(int x_index, int z_index) const {
    return height_map[x_index][z_index];
}
float Terrain::getX() const { return world_x; }
float Terrain::getZ() const { return world_z; }

// ReloadTerrainChunk:
// Reloads a sample of the height-map by index, by calculating the x,z
// coordinates of that sample.
void Terrain::reloadTerrainChunk(UINT index_x, UINT index_z) {
    // For our sample index, calculate the x,z coordinates of where we want to
    // sample.
    const float x = world_x + DISTANCE_BETWEEN_SAMPLES * index_x;
    const float z = world_z + DISTANCE_BETWEEN_SAMPLES * index_z;

    constexpr float ROUGHNESS = 0.0035f;
    const float noise =
        PerlinNoise::octaveNoise2D(x * ROUGHNESS, z * ROUGHNESS, 4, 6);
    const float height = noise * HEIGHT_MAP_Y_HEIGHT;

    height_map[index_x][index_z] = height;
}

// CalculateXZCoordinate:
// Calculates and returns the x,z coordinates for a heightmap sample
float Terrain::calculateXCoordinate(int x_index) const {
    return world_x + DISTANCE_BETWEEN_SAMPLES * x_index;
}
float Terrain::calculateZCoordinate(int z_index) const {
    return world_z + DISTANCE_BETWEEN_SAMPLES * z_index;
}

// SampleTerrainHeight:
// Samples the height-map at a specific x,z value. Returns -1 if the x,z
// coordinates are outside of the terrain.
float Terrain::sampleTerrainHeight(float x, float z) const {
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

} // namespace Datamodel
} // namespace Engine