#include "WaterSurface.h"

#include "math/Compute.h"

namespace Engine {
namespace Graphics {
WaterSurface::WaterSurface() = default;
WaterSurface::~WaterSurface() = default;

// LevelOfDetail:
// Determines the level of detail for the water surface.
// As distance increases, the level of detail we need should decrease
int WaterSurface::levelOfDetail(float dist) {
    constexpr int LOD_MAX = 9;
    const float DIST_MAX = size * 15.f;

    const float a = LOD_MAX / (DIST_MAX * DIST_MAX);
    const float b = -2 * LOD_MAX / DIST_MAX;
    const float c = LOD_MAX;

    return (dist * (a * dist + b)) + c;
}

// GenerateSurfaceMesh
// Generates a surface mesh for the water. Does this by creating a
// plane, which has been subdivided N times. The vertices of this
// plane will be transformed in the vertex shader to the appropriate
// water heights.
void WaterSurface::generateSurfaceMesh(ID3D11Device* device, float _size,
                                       int subdivisions) {
    MeshBuilder builder = MeshBuilder(BUILDER_POSITION);
    size = _size;

    /*
    a1 -- a2 -- a3
    |     |     |
    b1 -- b2 -- b3
    |     |     |
    c1 -- c2 -- c3
    We create these vertices and subdivide them.
    When subdividing, the 4 corners will be given in CW order, with
    the first corner given being the closest to the center
    */
    const UINT a1 = builder.addVertex(Vector3(-size, 0, size));
    const UINT a2 = builder.addVertex(Vector3(0, 0, size));
    const UINT a3 = builder.addVertex(Vector3(size, 0, size));

    const UINT b1 = builder.addVertex(Vector3(-size, 0, 0));
    const UINT b2 = builder.addVertex(Vector3(0, 0, 0));
    const UINT b3 = builder.addVertex(Vector3(size, 0, 0));

    const UINT c1 = builder.addVertex(Vector3(-size, 0, -size));
    const UINT c2 = builder.addVertex(Vector3(0, 0, -size));
    const UINT c3 = builder.addVertex(Vector3(size, 0, -size));

    subdivideSurface(builder, b2, b1, a1, a2, 0);
    subdivideSurface(builder, b2, a2, a3, b3, 0);
    subdivideSurface(builder, b2, c2, c1, b1, 0);
    subdivideSurface(builder, b2, b3, c3, c2, 0);

    surface_mesh = builder.generateMesh(device);
}

// Given a quad formed by points a,b,c,d (in a rectangle)
// a -- b
// |    |
// d -- c
// We assume that vertex a is closest to the center of the plane's surface
// Recursivly subdivides this plane into quadrants.
void WaterSurface::subdivideSurface(MeshBuilder& builder, UINT a, UINT b,
                                    UINT c, UINT d, int depth) {
    // Get the position of A, and see how far it is from the center.
    const Vector3 A = builder.getPosition(a);
    const float dist = sqrtf(A.x * A.x + A.z * A.z); // Manual since y = 0

    // We plug this distance into a function, telling us the level of
    // subdivisions we want at that distance. This tells us if we need to
    // subdivide more or not. This should be a decreasing function
    const int lod = levelOfDetail(dist);

    // Recursive Case:
    // Subdivide further
    if (depth < lod) {
        const Vector3 ab = (A + builder.getPosition(b)) / 2.f;
        const Vector3 bc =
            (builder.getPosition(b) + builder.getPosition(c)) / 2.f;
        const Vector3 cd =
            (builder.getPosition(c) + builder.getPosition(d)) / 2.f;
        const Vector3 da = (builder.getPosition(d) + A) / 2.f;
        const Vector3 center = (ab + cd) / 2.f;

        const UINT ab_i = builder.addVertex(ab);
        const UINT bc_i = builder.addVertex(bc);
        const UINT cd_i = builder.addVertex(cd);
        const UINT da_i = builder.addVertex(da);
        const UINT center_i = builder.addVertex(center);

        subdivideSurface(builder, a, ab_i, center_i, da_i, depth + 1);
        subdivideSurface(builder, ab_i, b, bc_i, center_i, depth + 1);
        subdivideSurface(builder, da_i, center_i, cd_i, d, depth + 1);
        subdivideSurface(builder, center_i, bc_i, c, cd_i, depth + 1);
    }
    // Base Case:
    // Add triangles to make the quad
    else {
        builder.addTriangle(d, a, b);
        builder.addTriangle(c, d, b);
    }
}

// GenerateWaveConfig:
// Randomly generates the wave configurations for the engine.
void WaterSurface::generateWaveConfig(int wave_count) {
    wave_config.clear();
    num_waves = wave_count;

    float amplitude = 0.75f;
    float frequency = 0.1f;

    for (int i = 0; i < num_waves; i++) {
        WaveConfig config;

        const float theta = Random(-float(2 * PI), float(2 * PI));
        config.direction = Vector2(cosf(theta), sinf(theta));

        config.amplitude = amplitude;
        amplitude *= Random(0.83f, 0.99f);

        config.period = frequency;
        frequency *= Random(1.01f, 1.17f);

        wave_config.push_back(config);
    }
}

// --- Accessors ---
// GetSurfaceMesh:
const Mesh* WaterSurface::getSurfaceMesh() const { return surface_mesh; }

// GetNumWaves:
int WaterSurface::getNumWaves() const { return num_waves; }
const std::vector<WaveConfig>& WaterSurface::getWaveConfig() const {
    return wave_config;
}

} // namespace Graphics
} // namespace Engine