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

Terrain::Terrain(int x_offset, int z_offset) {
    // Initialize terrain data. Terrain will be from (0,0,0) to (2,2,z).
    // We sample between these values with Perlin noise, with the
    for (int i = 0; i < TERRAIN_CHUNK_X_SAMPLES; i++) {
        for (int j = 0; j < TERRAIN_CHUNK_Z_SAMPLES; j++) {
            // Calculate the corresponding (x,z) coordinate in the noise
            // function
            float x = i * TERRAIN_SIZE / (TERRAIN_CHUNK_X_SAMPLES - 1) +
                      x_offset * TERRAIN_SIZE;
            float z = j * TERRAIN_SIZE / (TERRAIN_CHUNK_Z_SAMPLES - 1) +
                      z_offset * TERRAIN_SIZE;

            // Take perlin noise. It specifies at what y coordinate the surface
            // should be.
            constexpr float roughness = 0.01f;
            float noise = PerlinNoise::octaveNoise2D(x * roughness, z * roughness, 4, 6);
            int coord = (int)(noise * TERRAIN_CHUNK_Y_SAMPLES);

            for (int k = 0; k < coord; k++)
                terrainData[i][j][k] = 1;
            for (int k = coord; k < TERRAIN_CHUNK_Y_SAMPLES; k++)
                terrainData[i][j][k] = -1;
        }
    }
}
Terrain::~Terrain() = default;

float Terrain::sample(UINT x, UINT y, UINT z) const {
    return terrainData[x][z][y];
}

float (*Terrain::getRawData())[TERRAIN_CHUNK_X_SAMPLES][TERRAIN_CHUNK_Z_SAMPLES]
                              [TERRAIN_CHUNK_Y_SAMPLES] {
    return &terrainData;
}

} // namespace Datamodel
} // namespace Engine