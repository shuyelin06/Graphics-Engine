#pragma once

#include "math/Vector3.h"

constexpr float HEIGHT_MAP_XZ_SIZE = 25.f;
constexpr float HEIGHT_MAP_Y_HEIGHT = 100.f;

constexpr int HEIGHT_MAP_XZ_SAMPLES = 10;

constexpr float DISTANCE_BETWEEN_SAMPLES =
    HEIGHT_MAP_XZ_SIZE / (HEIGHT_MAP_XZ_SAMPLES - 1);

namespace Engine {
using namespace Math;

namespace Graphics {
class VisualTerrain;
};

namespace Datamodel {
typedef unsigned int UINT;

// Terrain Class:
// Stores a height-map representing the terrain in the engine.
class TerrainChunk {
  private:
    // Stores the bottom x,z world coordinates for the terrain specifying the world coordinates\
    // of the terrain
    float world_x, world_z;

    // 2D heightmap that stores the y height of the terrain in the X,Z
    // directions
    float height_map[HEIGHT_MAP_XZ_SAMPLES][HEIGHT_MAP_XZ_SAMPLES];

    // Physics / Graphical Interfaces
    // Visual Component for the Terrain
    Graphics::VisualTerrain* visual_terrain;

  public:
    TerrainChunk(float world_x, float world_z);
    ~TerrainChunk();

    // Get properties of the terrain
    float getX() const;
    float getZ() const;

    // Get the height of the terrain given a x,z coordinate. Uses bi-linear
    // interpolation to sample this height.
    float sampleTerrainHeight(float x, float z) const;

    // Bind terrain to components from the visual and physics system.
    void bindVisualTerrain(Graphics::VisualTerrain* visual_terrain);
    bool hasVisualTerrain() const;

  private:
    // Reload the data of a portion of the terrain chunk by index.
    void reloadTerrainChunk(UINT index_x, UINT index_z);
};

} // namespace Datamodel
} // namespace Engine
