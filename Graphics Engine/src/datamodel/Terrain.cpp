#include "Terrain.h"

#include <assert.h>

#include <map>

#include "math/Compute.h"
#include "math/PerlinNoise.h"
#include "math/Matrix3.h"
#include "math/Triangle.h"

#if defined(TERRAIN_DEBUG)
#include "rendering/VisualDebug.h"
#endif

namespace Engine {
namespace Datamodel {

Terrain::Terrain() {
    // Initialize terrain data. Terrain will be from (0,0,0) to (2,2,z).
    // We sample between these values with Perlin noise, with the 
    for (int i = 0; i < CHUNK_X_SAMPLES; i++) {
        for (int j = 0; j < CHUNK_Y_SAMPLES; j++) {
            // Calculate the corresponding (x,y) coordinate in the noise function
            float x = i * TERRAIN_SIZE / CHUNK_X_SAMPLES;
            float y = j * TERRAIN_SIZE / CHUNK_Y_SAMPLES;

            // Take perlin noise. It specifies at what z coordinate should the surface be.
            float noise = PerlinNoise::noise2D(x,y);
            int coord = (int) (noise * CHUNK_Z_SAMPLES);
            
            for (int k = 0; k < coord; k++)
                terrainData[i][j][k] = -1;
            for (int k = coord; k < CHUNK_Z_SAMPLES; k++)
                terrainData[i][j][k] = 1;
        }
    }

#if defined(TERRAIN_DEBUG)
    for (int i = 0; i < CHUNK_X_SIZE; i++) {
        for (int j = 0; j < CHUNK_Y_SIZE; j++) {
            for (int k = 0; k < CHUNK_Z_SIZE; k++) {
                if (terrainData[i][j][k] < 0)
                    Graphics::VisualDebug::DrawPoint(Vector3(i, j, k) *
                                                         CHUNK_VOXEL_SIZE,
                                                     1.5f, Color::Red(), -1);
                else
                    Graphics::VisualDebug::DrawPoint(Vector3(i, j, k) *
                                                         CHUNK_VOXEL_SIZE,
                                                     1.5f, Color::Green(), -1);
            }
        }
    }
#endif
}
Terrain::~Terrain() = default;

float Terrain::sample(UINT x, UINT y, UINT z) const
{
    return terrainData[x][y][z];
}

} // namespace Datamodel
} // namespace Engine