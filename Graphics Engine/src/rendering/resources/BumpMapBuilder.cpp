#include "BumpMapBuilder.h"

#include "math/PerlinNoise.h"
#include "math/Vector3.h"

#include <assert.h>

namespace Engine {
using namespace Math;

namespace Graphics {
BumpMapBuilder::BumpMapBuilder(UINT width, UINT height)
    : TextureBuilder(width - 2, height - 2) {
    heightmap_width = width;
    heightmap_height = height;
    heightmap.resize(heightmap_width * heightmap_height, 0);
}
BumpMapBuilder::~BumpMapBuilder() = default;

// Generate:
// Iterates over the texture, generating the normals
// from the heightmap.
Texture* BumpMapBuilder::generate(ID3D11Device* device, bool editable) {
    computeNormals();
    return TextureBuilder::generate(device, editable);
}
// Update:
// Generates the normals, and updates an existing texture
void BumpMapBuilder::update(Texture* texture, ID3D11DeviceContext* context) {
    computeNormals();
    TextureBuilder::update(texture, context);
}

// SetHeight:
// Sets the height at a specified x,y coordinate.
void BumpMapBuilder::setHeight(int x, int y, float val) {
    assert(0 <= x && x < heightmap_width);
    assert(0 <= y && y < heightmap_height);

    heightmap[index(x, y)] = val;
}
// SamplePerlinNoise:
// Sets the builder's height with the Perlin Noise function
void BumpMapBuilder::samplePerlinNoise(unsigned int seed, float freq,
                                       float amplitude) {
    PerlinNoise noise = PerlinNoise(seed);

    for (int i = 0; i < heightmap_width; i++) {
        for (int j = 0; j < heightmap_height; j++) {
            const float val = noise.octaveNoise2D(freq * i, freq * j, 5, 0.75f);
            setHeight(i, j, amplitude * val);
        }
    }
}

// ComputeNormals:
// Computes the normals for the bump map and places them in the texture
// builder's RGB properties.
void BumpMapBuilder::computeNormals() {
    // Iterate through our height map and calculate our normals.
    // We do this by sampling the tangent vectors in the x and y
    // directions, and crossing the tangents.
    for (int i = 1; i < heightmap_width - 1; i++) {
        for (int j = 1; j < heightmap_height - 1; j++) {
            const float x_diff =
                heightmap[index(i + 1, j)] - heightmap[index(i - 1, j)];
            const float y_diff =
                heightmap[index(i, j + 1)] - heightmap[index(i, j - 1)];

            const Vector3 tangent_x = Vector3(0.f, x_diff / 2.f, 1.f);
            const Vector3 tangent_y = Vector3(1.f, y_diff / 2.f, 0.f);

            // Calculate my normal
            Vector3 normal = tangent_x.cross(tangent_y);
            normal.inplaceNormalize();

            // Convert to the color space
            TextureColor color;
            color.r = (uint8_t)((normal.x * 0.5f + 0.5f) * 255.f);
            color.g = (uint8_t)((normal.y * 0.5f + 0.5f) * 255.f);
            color.b = (uint8_t)((normal.z * 0.5f + 0.5f) * 255.f);

            setColor(i - 1, j - 1, color);
        }
    }
}

// IndexOf:
// Calculates the index of a (x,y) index
int BumpMapBuilder::index(int x, int y) { return x * heightmap_height + y; }

} // namespace Graphics
} // namespace Engine