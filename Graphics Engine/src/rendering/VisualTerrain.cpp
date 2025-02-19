#include "VisualTerrain.h"

constexpr int SAMPLE_COUNT = 20;

namespace Engine {
namespace Graphics {
VisualTerrain::VisualTerrain(TerrainChunk* _terrain, MeshBuilder* mesh_builder)
    : terrain(_terrain) {
    terrain_mesh = generateTerrainMesh(*mesh_builder);

    markedToDestroy = false;
}

VisualTerrain::~VisualTerrain() = default;

void VisualTerrain::destroy() { markedToDestroy = true; }

bool VisualTerrain::markedForDestruction() const {
    return markedToDestroy;
}

// GenerateTerrainMesh:
// Generates the mesh for the terrain
Mesh* VisualTerrain::generateTerrainMesh(MeshBuilder& builder) {
    const Vector2 XZ_MIN = Vector2(terrain->getX(), terrain->getZ());
    const Vector2 XZ_MAX = Vector2(terrain->getX() + HEIGHT_MAP_XZ_SIZE,
                                   terrain->getZ() + HEIGHT_MAP_XZ_SIZE);

    for (int i = 0; i < SAMPLE_COUNT; i++) {
        for (int j = 0; j < SAMPLE_COUNT; j++) {
            const float x =
                (XZ_MAX.u - XZ_MIN.u) * i / (SAMPLE_COUNT - 1) + XZ_MIN.u;
            const float z =
                (XZ_MAX.v - XZ_MIN.v) * j / (SAMPLE_COUNT - 1) + XZ_MIN.v;

            const float y = terrain->sampleTerrainHeight(x, z);

            builder.addVertex(Vector3(x, y, z));
        }
    }

    for (int i = 0; i < SAMPLE_COUNT; i++) {
        for (int j = 0; j < SAMPLE_COUNT; j++) {
            const int v0 = i * SAMPLE_COUNT + j;
            const int v1 = (i + 1) * SAMPLE_COUNT + j;
            const int v2 = i * SAMPLE_COUNT + (j + 1);
            const int v3 = (i - 1) * SAMPLE_COUNT + j;
            const int v4 = i * SAMPLE_COUNT + (j - 1);

            // NE triangle
            if (i < SAMPLE_COUNT - 1 && j < SAMPLE_COUNT - 1)
                builder.addTriangle(v0, v2, v1);

            // NW triangle
            if (i > 0 && j < SAMPLE_COUNT - 1)
                builder.addTriangle(v0, v3, v2);

            // SW triangle
            if (i > 0 && j > 0)
                builder.addTriangle(v0, v4, v3);

            // SE triangle
            if (i < SAMPLE_COUNT - 1 && j > 0)
                builder.addTriangle(v0, v1, v4);
        }
    }

    builder.regenerateNormals();
    return builder.generate();
}

} // namespace Graphics
} // namespace Engine