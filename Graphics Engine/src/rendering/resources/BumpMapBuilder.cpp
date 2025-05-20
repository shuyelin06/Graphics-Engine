#include "BumpMapBuilder.h"

#include "math/Vector3.h"

#include <assert.h>

namespace Engine {
using namespace Math;

namespace Graphics {
BumpMapBuilder::BumpMapBuilder(UINT width, UINT height)
    : TextureBuilder(width, height) {
    heightmap.resize(width * height, 0);
}
BumpMapBuilder::~BumpMapBuilder() = default;

// Generate:
// Iterates over the texture, generating the normals
// from the heightmap.
Texture* BumpMapBuilder::generate(ID3D11Device* device) {
    // Iterate through our height map and calculate our normals.
    // We do this by sampling the tangent vectors in the x and y
    // directions, and crossing the tangents.
    for (int i = 1; i < width - 1; i++) {
        for (int j = 1; j < height - 1; j++) {
            const float x_diff =
                heightmap[index(i + 1, j)] - heightmap[index(i - 1, j)];
            const float y_diff =
                heightmap[index(i, j + 1)] - heightmap[index(i, j - 1)];

            const Vector3 tangent_x = Vector3(1.f, 0.f, x_diff / 2.f);
            const Vector3 tangent_y = Vector3(0.f, 1.f, y_diff / 2.f);

            // Calculate my normal
            Vector3 normal = tangent_x.cross(tangent_y);
            normal.inplaceNormalize();

            // Convert to the color space
            TextureColor color;
            color.r = (uint8_t)((normal.x * 0.5f + 0.5f) * 255.f);
            color.b = (uint8_t)((normal.y * 0.5f + 0.5f) * 255.f);
            color.b = (uint8_t)((normal.z * 0.5f + 0.5f) * 255.f);

            setColor(i, j, color);
        }
    }

    return TextureBuilder::generate(device);
}

// SetHeight:
// Sets the height at a specified x,y coordinate.
void BumpMapBuilder::setHeight(int x, int y, float val) {
    assert(0 <= x && x < width);
    assert(0 <= y && y < height);

    heightmap[index(x, y)] = val;
}

// IndexOf:
// Calculates the index of a (x,y) index
int BumpMapBuilder::index(int x, int y) { return x * height + y; }

} // namespace Graphics
} // namespace Engine