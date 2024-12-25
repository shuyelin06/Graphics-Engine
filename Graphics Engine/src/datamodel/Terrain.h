#pragma once

#include "datamodel/TerrainConfig.h"
#include "math/Vector3.h"

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
    float terrainData[TERRAIN_CHUNK_X_SAMPLES][TERRAIN_CHUNK_Z_SAMPLES]
                     [TERRAIN_CHUNK_Y_SAMPLES];

  public:
    Terrain(int x_offset, int z_offset);
    ~Terrain();

    float sample(UINT x, UINT y, UINT z) const;
    float (*getRawData())[TERRAIN_CHUNK_X_SAMPLES][TERRAIN_CHUNK_Z_SAMPLES]
                         [TERRAIN_CHUNK_Y_SAMPLES];
};

} // namespace Datamodel
} // namespace Engine
