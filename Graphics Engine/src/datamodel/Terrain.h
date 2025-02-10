#pragma once

#include "math/Vector3.h"

constexpr float HEIGHT_MAP_XZ_SIZE = 200.f;
constexpr float HEIGHT_MAP_Y_HEIGHT = 100.f;

constexpr int HEIGHT_MAP_XZ_SAMPLES = 20;

constexpr float DISTANCE_BETWEEN_SAMPLES = HEIGHT_MAP_XZ_SIZE / HEIGHT_MAP_XZ_SAMPLES;

namespace Engine {
using namespace Math;
namespace Datamodel {

typedef unsigned int UINT;

// Terrain Class:
// Stores a height-map representing the terrain in the engine.
class Terrain {
  private:
    // Stores the bottom x,z world coordinates for the terrain specifying the world coordinates\
    // of the terrain
    float world_x, world_z;

    // 2D heightmap that stores the y height of the terrain in the X,Z directions
    float height_map[HEIGHT_MAP_XZ_SAMPLES][HEIGHT_MAP_XZ_SAMPLES];

  public:
    Terrain(float world_x, float world_z);
    
    // Get the x,z coordinates on which the Terrain is based
    float getTerrainHeight(int x_index, int z_index) const;
    float getX() const;
    float getZ() const;

    // Calculates and returns the x,z coordinates for a heightmap sample
    float calculateXCoordinate(int x_index) const;
    float calculateZCoordinate(int z_index) const;

    // Get the height of the terrain given a x,z coordinate
    float sampleTerrainHeight(float x, float z) const;

    // Reload a terrain chunk by index, and samples based on Does this by sampling
    void reloadTerrainChunk(UINT index_x, UINT index_z);    
};

} // namespace Datamodel
} // namespace Engine
