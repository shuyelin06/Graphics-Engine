#include "ChunkBuilderJob.h"

#include "datamodel/terrain/Terrain.h"
#include "datamodel/terrain/TerrainConfig.h"

#include "MarchingCube.h"

// Buggy?
constexpr bool GENERATE_SKIRT = false;

namespace Engine {
using namespace Datamodel;

namespace Graphics {
ChunkBuilderJob::ChunkBuilderJob(MeshPool* terrain_pool)
    : async_lock(), builder(terrain_pool), stopwatch() {
    status = Inactive;
}

void ChunkBuilderJob::buildChunkMesh() {
    std::unique_lock lock(async_lock);

    stopwatch.Reset();
    vertex_map.clear();
    border_triangles.clear();

    // Use Marching Cubes to generate my terrain mesh.
    MarchingCube marching_cube = MarchingCube();
    int num_triangles;
    Triangle triangles[12];

    // -1 because of Marching Cube
    for (int i = 0; i < TERRAIN_SAMPLES_PER_CHUNK + 2 - 1; i++) {
        for (int j = 0; j < TERRAIN_SAMPLES_PER_CHUNK + 2 - 1; j++) {
            for (int k = 0; k < TERRAIN_SAMPLES_PER_CHUNK + 2 - 1; k++) {
                // Load the data into my marching cube
                // clang-format off
                marching_cube.updateData(
                    data[i][j][k],              data[i + 1][j][k], 
                    data[i + 1][j + 1][k],      data[i][j + 1][k], 
                    data[i][j][k + 1],          data[i + 1][j][k + 1],
                    data[i + 1][j + 1][k + 1],  data[i][j + 1][k + 1]);
                // clang-format on

                // Generate my mesh for this cube
                num_triangles = 0;
                marching_cube.generateSurface(triangles, &num_triangles);

                // Offset and scale the triangles so
                // they correspond to this chunk's world space position.
                assert(num_triangles <= 12);

                const Vector3 mcube_pos = Vector3(i - 1, j - 1, k - 1);
                const float mcube_size =
                    chunk_size / (TERRAIN_SAMPLES_PER_CHUNK - 1);

                const bool is_external_triangle =
                    i == 0 || j == 0 || k == 0 ||
                    i == TERRAIN_SAMPLES_PER_CHUNK ||
                    j == TERRAIN_SAMPLES_PER_CHUNK ||
                    k == TERRAIN_SAMPLES_PER_CHUNK;
                const bool is_border_triangle = 
                    i == 1 || j == 1 || k == 1 ||
                    i == TERRAIN_SAMPLES_PER_CHUNK - 1 ||
                    j == TERRAIN_SAMPLES_PER_CHUNK - 1 ||
                    k == TERRAIN_SAMPLES_PER_CHUNK - 1;

                for (int tri = 0; tri < num_triangles; tri++) {
                    // 1) Locally offset the triangles to the marching cube
                    // position in chunk
                    // 2) Scale to marching cube size in world
                    // space
                    // 3) Offset to chunk world position.
                    Triangle& triangle = triangles[tri];

                    triangle.vertex(0) =
                        (triangle.vertex(0) + mcube_pos) * mcube_size +
                        chunk_position;
                    triangle.vertex(1) =
                        (triangle.vertex(1) + mcube_pos) * mcube_size +
                        chunk_position;
                    triangle.vertex(2) =
                        (triangle.vertex(2) + mcube_pos) * mcube_size +
                        chunk_position;

                    // 3 Cases:
                    // 1) Triangle is outside of the chunk (external triangle).
                    //    These triangles only exist to create smooth normals
                    //    between chunks, so we add them to the builder before
                    //    normal generation, and remove them after.
                    // 2) Triangle is on the chunk border (border triangle).
                    //    We need to generate a skirt for these triangles so
                    //    that LOD transitions are seamless. We add the triangle
                    //    and the skirt as well.
                    // 3) Triangle is inside the chunk. We just add the
                    //    triangle.
                    if (is_external_triangle) {
                        border_triangles.push_back(triangle);
                    } else if (GENERATE_SKIRT && is_border_triangle) {
                        // Generate my skirt. It is the triangle's vertices
                        // offset INTO the surface, AKA offset along the
                        // triangle normal.
                        Vector3 skirt_verts[3] = {triangle.vertex(0),
                                                  triangle.vertex(1),
                                                  triangle.vertex(2)};

                        constexpr float SKIRT_OFFSET = 5.f;
                        const Vector3 normal = triangle.normal();
                        skirt_verts[0] += normal * SKIRT_OFFSET;
                        skirt_verts[1] += normal * SKIRT_OFFSET;
                        skirt_verts[2] += normal * SKIRT_OFFSET;

                        // Connect these vertices to our triangle.
                        loadQuadIntoBuilder(skirt_verts[1], triangle.vertex(1),
                                            triangle.vertex(0), skirt_verts[0]);
                        loadQuadIntoBuilder(skirt_verts[0], triangle.vertex(0),
                                            triangle.vertex(2), skirt_verts[2]);
                        loadQuadIntoBuilder(skirt_verts[2], triangle.vertex(2),
                                            triangle.vertex(1), skirt_verts[1]);

                        loadTriangleIntoBuilder(triangle);
                    }
                    // Triangles in the chunk
                    else {
                        loadTriangleIntoBuilder(triangle);
                    }
                }
            }
        }
    }

    // Now, add our border triangles. These are needed so that our normals
    // between chunks are coherent. We remove them later to prevent z-fighting.
    for (auto& triangle : border_triangles) {
        loadTriangleIntoBuilder(triangle);
    }

    // Generate our normals, and remove the border triangles so that we don't
    // have z-fighting
    builder.regenerateNormals();
    builder.popTriangles(border_triangles.size());

    // Add a skirt for the chunk so that we have a smooth transition between
    // LODs
    // TODO:
    // 1: Do this
    // 2: Do it only for the boundary between LODs

    // Finished: Mark time taken so we can track performance.
    time_taken = stopwatch.Duration();

    status = Done;
}

// Helpers used by the asynchronous function
void ChunkBuilderJob::loadQuadIntoBuilder(const Vector3& a, const Vector3& b,
                                          const Vector3& c, const Vector3& d) {
    unsigned int i0, i1, i2, i3;
    i0 = builder.addVertex(a);
    i1 = builder.addVertex(b);
    i2 = builder.addVertex(c);
    i3 = builder.addVertex(d);
    builder.addTriangle(i0, i1, i2);
    builder.addTriangle(i2, i3, i0);
}

void ChunkBuilderJob::loadTriangleIntoBuilder(const Triangle& triangle) {
    unsigned int i0, i1, i2;
    i0 = loadVertexIntoBuilder(triangle.vertex(0));
    i1 = loadVertexIntoBuilder(triangle.vertex(1));
    i2 = loadVertexIntoBuilder(triangle.vertex(2));
    builder.addTriangle(i0, i1, i2);
}

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