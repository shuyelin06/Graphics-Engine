#include "ChunkBuilderJob.h"

#include "datamodel/terrain/Terrain.h"
#include "datamodel/terrain/TerrainConfig.h"

#include "MarchingCube.h"

namespace Engine {
using namespace Datamodel;

namespace Graphics {
ChunkBuilderJob::ChunkBuilderJob(MeshPool* terrain_pool)
    : async_lock(), builder(terrain_pool) {
    status = Inactive;
}

void ChunkBuilderJob::buildChunkMesh() {
    std::unique_lock lock(async_lock);

    // Use Marching Cubes to generate my terrain mesh.
    unsigned int i0, i1, i2;

    const float CHUNK_OFFSET = chunk_size / (TERRAIN_SAMPLES_PER_CHUNK - 1);
    const float x = chunk_position.x;
    const float y = chunk_position.y;
    const float z = chunk_position.z;

    MarchingCube marching_cube = MarchingCube();
    int num_triangles;
    Triangle triangles[12];

    border_triangles.clear();

    for (int i = 0; i < TERRAIN_SAMPLES_PER_CHUNK + 2 - 1; i++) {
        for (int j = 0; j < TERRAIN_SAMPLES_PER_CHUNK + 2 - 1; j++) {
            for (int k = 0; k < TERRAIN_SAMPLES_PER_CHUNK + 2 - 1; k++) {
                // Load the data into my marching cube
                marching_cube.updateData(
                    data[i][j][k], data[i + 1][j][k], data[i + 1][j + 1][k],
                    data[i][j + 1][k], data[i][j][k + 1], data[i + 1][j][k + 1],
                    data[i + 1][j + 1][k + 1], data[i][j + 1][k + 1]);

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
                        i == TERRAIN_SAMPLES_PER_CHUNK ||
                        j == TERRAIN_SAMPLES_PER_CHUNK ||
                        k == TERRAIN_SAMPLES_PER_CHUNK;

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

    status = Done;
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