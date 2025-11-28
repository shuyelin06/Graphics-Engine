#include "ChunkBuilderJob.h"

#include "datamodel/terrain/Terrain.h"
#include "datamodel/terrain/TerrainConfig.h"

#include "MarchingCube.h"

namespace Engine {
using namespace Datamodel;

namespace Graphics {
ChunkBuilderJob::ChunkBuilderJob(MeshPool* terrain_pool)
    : async_lock(), builder(terrain_pool) {
    active = false;
}

void ChunkBuilderJob::loadChunkData(const TerrainChunk& data,
                                    const ChunkIndex& arr_index) {
    chunk_copy = data;
    chunk_array_index = arr_index;

    vertex_map.clear();
    border_triangles.clear();

    builder.reset();
    builder.addLayout(POSITION);
    builder.addLayout(NORMAL);
}
void ChunkBuilderJob::buildChunkMesh() {
    std::unique_lock lock(async_lock);
    active = true;

    // Use Marching Cubes to generate my terrain mesh.
    unsigned int i0, i1, i2;

    constexpr float CHUNK_OFFSET =
        TERRAIN_CHUNK_SIZE / (TERRAIN_CHUNK_SAMPLES - 1);
    const float x = chunk_copy.chunk_x * TERRAIN_CHUNK_SIZE;
    const float y = chunk_copy.chunk_y * TERRAIN_CHUNK_SIZE;
    const float z = chunk_copy.chunk_z * TERRAIN_CHUNK_SIZE;

    MarchingCube marching_cube = MarchingCube();
    int num_triangles;
    Triangle triangles[12];

    for (int i = 0; i < TERRAIN_CHUNK_SAMPLES + 2 - 1; i++) {
        for (int j = 0; j < TERRAIN_CHUNK_SAMPLES + 2 - 1; j++) {
            for (int k = 0; k < TERRAIN_CHUNK_SAMPLES + 2 - 1; k++) {
                // Load the data into my marching cube
                marching_cube.updateData(
                    chunk_copy.data[i][j][k], chunk_copy.data[i + 1][j][k],
                    chunk_copy.data[i + 1][j + 1][k],
                    chunk_copy.data[i][j + 1][k], chunk_copy.data[i][j][k + 1],
                    chunk_copy.data[i + 1][j][k + 1],
                    chunk_copy.data[i + 1][j + 1][k + 1],
                    chunk_copy.data[i][j + 1][k + 1]);

                // Generate my mesh for this cube
                num_triangles = 0;
                marching_cube.generateSurface(triangles, &num_triangles);

                // Offset and scale the triangles so
                // they correspond to this chunk's world space position.
                assert(num_triangles <= 12);
                for (int tri = 0; tri < num_triangles; tri++) {
                    Triangle& triangle = triangles[tri];

                    triangle.vertex(0).set(
                        (triangle.vertex(0) + Vector3(i - 1, j - 1, k - 1)) *
                            CHUNK_OFFSET +
                        Vector3(x, y, z));
                    triangle.vertex(1).set(
                        (triangle.vertex(1) + Vector3(i - 1, j - 1, k - 1)) *
                            CHUNK_OFFSET +
                        Vector3(x, y, z));
                    triangle.vertex(2).set(
                        (triangle.vertex(2) + Vector3(i - 1, j - 1, k - 1)) *
                            CHUNK_OFFSET +
                        Vector3(x, y, z));

                    // We add border triangles later. They only exist for the
                    // normals, and should be removed after
                    const bool is_border_triangle =
                        i == 0 || j == 0 || k == 0 ||
                        i == TERRAIN_CHUNK_SAMPLES ||
                        j == TERRAIN_CHUNK_SAMPLES ||
                        k == TERRAIN_CHUNK_SAMPLES;

                    if (is_border_triangle) {
                        border_triangles.push_back(triangle);
                    }
                    // Triangles in the chunk
                    else {
                        i0 = loadVertexIntoBuilder(triangle.vertex(0));
                        i1 = loadVertexIntoBuilder(triangle.vertex(1));
                        i2 = loadVertexIntoBuilder(triangle.vertex(2));
                        builder.addTriangle(i0, i1, i2);
                    }
                }
            }
        }
    }

    // Now, add our border triangles. These are needed so that our normals
    // between chunks are coherent. We remove them later to prevent z-fighting.
    for (auto& triangle : border_triangles) {
        i0 = loadVertexIntoBuilder(triangle.vertex(0));
        i1 = loadVertexIntoBuilder(triangle.vertex(1));
        i2 = loadVertexIntoBuilder(triangle.vertex(2));
        builder.addTriangle(i0, i1, i2);
    }

    // Generate our normals, and remove the border triangles so that we don't
    // have z-fighting
    builder.regenerateNormals();
    builder.popTriangles(border_triangles.size());
}

// Helpers used by the asynchronous function
unsigned int ChunkBuilderJob::loadVertexIntoBuilder(const Vector3& vertex) {
    unsigned int index;
    if (vertex_map.contains(vertex))
        index = vertex_map[vertex];
    else {
        index = builder.addVertex(vertex);
        vertex_map[vertex] = index;
    }
    return index;
}

} // namespace Graphics
} // namespace Engine