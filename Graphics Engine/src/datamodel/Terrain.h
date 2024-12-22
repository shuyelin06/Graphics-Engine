#pragma once

#include "math/Vector3.h"

// Enables debug rendering for the terrain
// #define TERRAIN_DEBUG

constexpr int CHUNK_X_SAMPLES = 31;
constexpr int CHUNK_Y_SAMPLES = 51;
constexpr int CHUNK_Z_SAMPLES = 31;

constexpr float TERRAIN_SIZE = 75.f;
constexpr float TERRAIN_HEIGHT = 15.f;

namespace Engine {
using namespace Math;
namespace Datamodel {

typedef unsigned int UINT;

// Terrain Class:
// Represents a chunk of terrain that can be sampled and rendered.
// Terrain generation is done by running the Marching Cubes algorithm on some
// defined noise function.
// It generates the best surface for the data by assuming linear interpolation,
// and generating a surface where the data will approximately be 0. Data that is
// postive is assumed to be inside the surface, and data that is negative is
// assumed to be outside the surface.
class Terrain {
  private:
    // Stores a scalar field's values at vertices of each voxel within the
    // chunk.
    float terrainData[CHUNK_X_SAMPLES][CHUNK_Z_SAMPLES][CHUNK_Y_SAMPLES];

  public:
    Terrain();
    ~Terrain();

    float sample(UINT x, UINT y, UINT z) const;
};

} // namespace Datamodel
} // namespace Engine
