#include "WaterSurface.h"

#include <algorithm>
#include <queue>

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
struct IndexPair {
    int x, z;
    IndexPair(int _x, int _z) {
        x = _x;
        z = _z;
    }
};

// GenerateSurfaceMesh:
// Generates a water surface mesh. This is done in a way where the mesh, when
// scaled by a factor of 2, can seamlessly fit onto the next mesh. This lets us
// implement LODs for the water using instancing. To work, the generated mesh
// creates a "ring" like mesh in a single quadrant. This has a few implications:
// 1) The inner ring must be rendered separately. It is, however, included in
// the mesh in the first XX triangles 2) Every render of this mesh must be
// repeated 3 more times for each of the other 3 quadrants.
//    The mesh has to be rotated to fit the other quadrants.
// These requirements make it really convenient to render this mesh with
// instancing. Width determines how many tiles this LOD layer will have before
// transitioning to the next. The larger the width, the larger an LOD will be.
void WaterSurface::generateSurfaceMesh(ID3D11Device* device, int width) {

    MeshBuilder builder = MeshBuilder();
    builder.addLayout(POSITION);

    // Saves how many of the initial triangles belong to the inner mesh that
    // fills in the ring. This mesh should only be rendered once, for the
    // closest water surface mesh.
    num_inner_tri = 0;

    std::queue<IndexPair> queue;

    // We generate our mesh in 2 passes. The first pass will generate the inner
    // mesh that fills in the ring. This mesh will be in the first XX
    // triangles of the index buffer, and should only be rendered for the first
    // water LOD.
    // Generation works by radially traversing outwards in a grid-like mannner,
    // where each tile is split into 4 quadrants as so:
    //
    // B -- BC -- C
    // |    |     |
    // AB - CEN - CD
    // |    |     |
    // A -- DA -- D
    //
    // The choice of what points in that tile to include are given in the loop
    // below.
    queue.push({0, 0});

    while (!queue.empty()) {
        const int num_elements = queue.size();
        for (int i = 0; i < num_elements; i++) {
            IndexPair pair = queue.front();
            queue.pop();

            // 1) If radius < width, we need to triangulate the tile into 4
            // quadrants. 2) If radius == widt, we need to only triangulate the
            // closest quadrants
            //    to (0,0)
            const int radius = max(pair.x, pair.z);
            if (radius <= width) {
                const Vector3 A = Vector3(pair.x, 0.f, pair.z);
                const Vector3 B = A + Vector3(0, 0, 1);
                const Vector3 C = B + Vector3(1, 0, 0);
                const Vector3 D = C + Vector3(0, 0, -1);

                const Vector3 AB = (A + B) / 2.f;
                const Vector3 BC = (B + C) / 2.f;
                const Vector3 CD = (C + D) / 2.f;
                const Vector3 DA = (D + A) / 2.f;
                const Vector3 CENTER = (AB + CD) / 2.f;

                // Here, we want to generate all 4 quadrants of each tile.
                if (radius < width) {
                    const UINT a = builder.addVertex(A);
                    const UINT ab = builder.addVertex(AB);
                    const UINT b = builder.addVertex(B);
                    const UINT bc = builder.addVertex(BC);
                    const UINT c = builder.addVertex(C);
                    const UINT cd = builder.addVertex(CD);
                    const UINT d = builder.addVertex(D);
                    const UINT da = builder.addVertex(DA);
                    const UINT cen = builder.addVertex(CENTER);

                    addQuad(builder, cen, da, a, ab);
                    addQuad(builder, cen, ab, b, bc);
                    addQuad(builder, cen, bc, c, cd);
                    addQuad(builder, cen, cd, d, da);
                    num_inner_tri += 8;
                }
                // Here, we only add the closest quadrants to (0,0)
                else {
                    const UINT a = builder.addVertex(A);
                    const UINT ab = builder.addVertex(AB);
                    const UINT cen = builder.addVertex(CENTER);
                    const UINT da = builder.addVertex(DA);
                    addQuad(builder, a, ab, cen, da);
                    num_inner_tri += 2;

                    if (pair.x < pair.z) {
                        const UINT cd = builder.addVertex(CD);
                        const UINT d = builder.addVertex(D);
                        addQuad(builder, cen, cd, d, da);
                        num_inner_tri += 2;
                    } else if (pair.z < pair.x) {
                        const UINT b = builder.addVertex(B);
                        const UINT bc = builder.addVertex(BC);
                        addQuad(builder, ab, b, bc, cen);
                        num_inner_tri += 2;
                    }
                }

                // Continue my iteration
                if (radius < width) {
                    if (pair.x <= pair.z)
                        queue.push({pair.x, pair.z + 1});
                    if (pair.z <= pair.x)
                        queue.push({pair.x + 1, pair.z});
                    if (pair.x == pair.z)
                        queue.push({pair.x + 1, pair.z + 1});
                }
            }
        }
    }

    // In the second pass, we will generate the actual LOD part of the mesh.
    queue.push({0, 0});

    while (!queue.empty()) {
        const int num_elements = queue.size();
        for (int i = 0; i < num_elements; i++) {
            IndexPair pair = queue.front();
            queue.pop();

            // The radius of our index from the center determines how we will
            // triangulate this tile.
            // 1) If radius < width, do nothing
            // 2) If radius == width, triangulate the furthest quadrants of the
            // tile 3) If width < radius < 2 * width, triangulate all 4
            // quadrants 4) If radius == 2 * width, triangulate the tile as 1
            // quad
            const int radius = max(pair.x, pair.z);
            if (width <= radius && radius <= 2 * width) {
                const Vector3 A = Vector3(pair.x, 0.f, pair.z);
                const Vector3 B = A + Vector3(0, 0, 1);
                const Vector3 C = B + Vector3(1, 0, 0);
                const Vector3 D = C + Vector3(0, 0, -1);

                const Vector3 AB = (A + B) / 2.f;
                const Vector3 BC = (B + C) / 2.f;
                const Vector3 CD = (C + D) / 2.f;
                const Vector3 DA = (D + A) / 2.f;
                const Vector3 CENTER = (AB + CD) / 2.f;

                if (radius == width) {
                    const UINT bc = builder.addVertex(BC);
                    const UINT c = builder.addVertex(C);
                    const UINT cd = builder.addVertex(CD);
                    const UINT cen = builder.addVertex(CENTER);
                    addQuad(builder, bc, c, cd, cen);

                    if (pair.x <= pair.z) {
                        const UINT ab = builder.addVertex(AB);
                        const UINT b = builder.addVertex(B);
                        addQuad(builder, ab, b, bc, cen);
                    }
                    if (pair.z <= pair.x) {
                        const UINT d = builder.addVertex(D);
                        const UINT da = builder.addVertex(DA);
                        addQuad(builder, cd, d, da, cen);
                    }
                }
                // Here, we need to blend our tile with the next LOD.
                // We do this by triangulating the tile in such a way that it
                // shares points along the border with the next LOD, and does
                // not generate any extra.
                else if (radius == 2 * width - 1) {
                    if (pair.x < pair.z) {
                        const UINT a = builder.addVertex(A);
                        const UINT ab = builder.addVertex(AB);
                        const UINT b = builder.addVertex(B);
                        const UINT c = builder.addVertex(C);
                        const UINT cd = builder.addVertex(CD);
                        const UINT d = builder.addVertex(D);
                        const UINT da = builder.addVertex(DA);
                        const UINT cen = builder.addVertex(CENTER);

                        builder.addTriangle(ab, b, cen);
                        builder.addTriangle(cen, b, c);
                        builder.addTriangle(cen, c, cd);

                        addQuad(builder, a, ab, cen, da);
                        addQuad(builder, da, cen, cd, d);
                    } else if (pair.z < pair.x) {
                        const UINT a = builder.addVertex(A);
                        const UINT ab = builder.addVertex(AB);
                        const UINT b = builder.addVertex(B);
                        const UINT bc = builder.addVertex(BC);
                        const UINT c = builder.addVertex(C);
                        const UINT d = builder.addVertex(D);
                        const UINT da = builder.addVertex(DA);
                        const UINT cen = builder.addVertex(CENTER);

                        builder.addTriangle(bc, c, cen);
                        builder.addTriangle(cen, c, d);
                        builder.addTriangle(cen, d, da);

                        addQuad(builder, ab, b, bc, cen);
                        addQuad(builder, a, ab, cen, da);
                    } else {
                        const UINT a = builder.addVertex(A);
                        const UINT ab = builder.addVertex(AB);
                        const UINT b = builder.addVertex(B);
                        const UINT c = builder.addVertex(C);
                        const UINT d = builder.addVertex(D);
                        const UINT da = builder.addVertex(DA);
                        const UINT cen = builder.addVertex(CENTER);

                        builder.addTriangle(cen, ab, b);
                        builder.addTriangle(cen, b, c);
                        builder.addTriangle(cen, c, d);
                        builder.addTriangle(cen, d, da);

                        addQuad(builder, a, ab, cen, da);
                    }
                } else if (radius == 2 * width) {
                    const UINT a = builder.addVertex(A);
                    const UINT b = builder.addVertex(B);
                    const UINT c = builder.addVertex(C);
                    const UINT d = builder.addVertex(D);

                    addQuad(builder, a, b, c, d);
                } else {
                    const UINT a = builder.addVertex(A);
                    const UINT ab = builder.addVertex(AB);
                    const UINT b = builder.addVertex(B);
                    const UINT bc = builder.addVertex(BC);
                    const UINT c = builder.addVertex(C);
                    const UINT cd = builder.addVertex(CD);
                    const UINT d = builder.addVertex(D);
                    const UINT da = builder.addVertex(DA);
                    const UINT cen = builder.addVertex(CENTER);

                    addQuad(builder, cen, da, a, ab);
                    addQuad(builder, cen, ab, b, bc);
                    addQuad(builder, cen, bc, c, cd);
                    addQuad(builder, cen, cd, d, da);
                }
            }

            if (radius <= 2 * width) {
                if (pair.x <= pair.z)
                    queue.push({pair.x, pair.z + 1});
                if (pair.z <= pair.x)
                    queue.push({pair.x + 1, pair.z});
                if (pair.x == pair.z)
                    queue.push({pair.x + 1, pair.z + 1});
            }
        }
    }

    surface_mesh = builder.generateMesh(device);
}

void WaterSurface::addQuad(MeshBuilder& builder, UINT a, UINT b, UINT c,
                           UINT d) {
    builder.addTriangle(a, b, c);
    builder.addTriangle(c, d, a);
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
int WaterSurface::getNumInnerTriangles() const { return num_inner_tri; }

// GetNumWaves:
int WaterSurface::getNumWaves() const { return num_waves; }
const std::vector<WaveConfig>& WaterSurface::getWaveConfig() const {
    return wave_config;
}

} // namespace Graphics
} // namespace Engine