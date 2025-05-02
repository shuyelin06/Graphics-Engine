#include "WaterSurface.h"

#include "math/Compute.h"

namespace Engine {
namespace Graphics {
WaterSurface::WaterSurface() = default;
WaterSurface::~WaterSurface() = default;

// GenerateSurfaceMesh
// Generates a surface mesh for the water. Does this by creating a
// plane, which has been subdivided N times. The vertices of this
// plane will be transformed in the vertex shader to the appropriate
// water heights.
static void subdivideSurface(MeshBuilder& builder, UINT a, UINT b, UINT c,
                             UINT d, int depth, int max_depth);
void WaterSurface::generateSurfaceMesh(ID3D11Device* device, float size,
                                       int subdivisions) {
    MeshBuilder builder = MeshBuilder(BUILDER_POSITION);

    const UINT a = builder.addVertex(Vector3(-size, 0, size));
    const UINT b = builder.addVertex(Vector3(size, 0, size));
    const UINT c = builder.addVertex(Vector3(size, 0, -size));
    const UINT d = builder.addVertex(Vector3(-size, 0, -size));
    subdivideSurface(builder, a, b, c, d, 0, subdivisions);

    surface_mesh = builder.generateMesh(device);
}

// Given a quad formed by points a,b,c,d (in a rectangle)
// a -- b
// |    |
// d -- c
// Recursivly subdivides this plane into quadrants.
void subdivideSurface(MeshBuilder& builder, UINT a, UINT b, UINT c, UINT d,
                      int depth, int max_depth) {
    // Base Case: Add triangles to make up the quad to mesh
    if (depth == max_depth) {
        builder.addTriangle(d, a, b);
        builder.addTriangle(c, d, b);
    }
    // Recursive Call: Add the vertices between a,b,c,d,
    // and recursively subdivide.
    else {
        const Vector3 ab =
            (builder.getPosition(a) + builder.getPosition(b)) / 2.f;
        const Vector3 bc =
            (builder.getPosition(b) + builder.getPosition(c)) / 2.f;
        const Vector3 cd =
            (builder.getPosition(c) + builder.getPosition(d)) / 2.f;
        const Vector3 da =
            (builder.getPosition(d) + builder.getPosition(a)) / 2.f;
        const Vector3 center = (ab + cd) / 2.f;

        const UINT ab_i = builder.addVertex(ab);
        const UINT bc_i = builder.addVertex(bc);
        const UINT cd_i = builder.addVertex(cd);
        const UINT da_i = builder.addVertex(da);
        const UINT center_i = builder.addVertex(center);

        subdivideSurface(builder, a, ab_i, center_i, da_i, depth + 1,
                         max_depth);
        subdivideSurface(builder, ab_i, b, bc_i, center_i, depth + 1,
                         max_depth);
        subdivideSurface(builder, da_i, center_i, cd_i, d, depth + 1,
                         max_depth);
        subdivideSurface(builder, center_i, bc_i, c, cd_i, depth + 1,
                         max_depth);
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