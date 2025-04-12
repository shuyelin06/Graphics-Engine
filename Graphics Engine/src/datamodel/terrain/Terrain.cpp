#include "Terrain.h"

#include <assert.h>

#include "MarchingCube.h"

#include "math/PerlinNoise.h"
#include "math/Triangle.h"

namespace Engine {
namespace Datamodel {
Terrain::Terrain() : noise_func(0) {
    center_x = center_y = center_z = 0;

    for (int i = 0; i < TERRAIN_CHUNK_COUNT; i++) {
        for (int j = 0; j < TERRAIN_CHUNK_COUNT; j++) {
            for (int k = 0; k < TERRAIN_CHUNK_COUNT; k++) {
                // TODO: Dynamic loading
                loadChunk(i, j, k, true);
            }
        }
    }
}

Terrain::~Terrain() {
    for (int i = 0; i < TERRAIN_CHUNK_COUNT; i++) {
        for (int j = 0; j < TERRAIN_CHUNK_COUNT; j++) {
            for (int k = 0; k < TERRAIN_CHUNK_COUNT; k++) {
                delete chunks[i][j][k];
            }
        }
    }
}

// --- Accessors ---
const std::vector<Triangle>& Terrain::getTrianglePool() const {
    return triangle_pool;
}

int Terrain::getCenterChunkX() const { return center_x; }
int Terrain::getCenterChunkY() const { return center_y; }
int Terrain::getCenterChunkZ() const { return center_z; }

const Chunk* Terrain::getChunk(int x_i, int y_i, int z_i) const {
    return chunks[x_i][y_i][z_i];
}

// ReloadTerrain:
void Terrain::reloadTerrain(float x, float y, float z) {
    // Calculate the chunk index that these x,y,z coordinates are in
    const int x_i = floor(x / TERRAIN_CHUNK_SIZE);
    const int y_i = floor(y / TERRAIN_CHUNK_SIZE);
    const int z_i = floor(z / TERRAIN_CHUNK_SIZE);

    // If the center chunk index has not changed, do nothing.
    if (center_x == x_i && center_y == y_i && center_z == z_i)
        return;

    // Find the x,z coordinates of the chunk that the center is located in.
    const int old_center_x = center_x;
    const int old_center_y = center_y;
    const int old_center_z = center_z;

    center_x = x_i;
    center_y = y_i;
    center_z = z_i;

    // We have a set of chunks around our old center. After moving our center,
    // we have a new set of chunks, but we don't want to regenerate chunks that
    // are the same. We will iterate through all chunks around our new center
    // and pull them from the old center chunks if possible.
    std::memset(chunks_helper, 0, sizeof(chunks_helper));

    for (int i = 0; i < TERRAIN_CHUNK_COUNT; i++) {
        for (int j = 0; j < TERRAIN_CHUNK_COUNT; j++) {
            for (int k = 0; k < TERRAIN_CHUNK_COUNT; k++) {
                // Find our chunk index on x,z
                const int old_index_x = i + center_x - old_center_x;
                const int old_index_y = j + center_y - old_center_y;
                const int old_index_z = k + center_z - old_center_z;

                const bool in_x_bounds =
                    0 <= old_index_x && old_index_x < TERRAIN_CHUNK_COUNT;
                const bool in_y_bounds =
                    0 <= old_index_y && old_index_y < TERRAIN_CHUNK_COUNT;
                const bool in_z_bounds =
                    0 <= old_index_z && old_index_z < TERRAIN_CHUNK_COUNT;

                // We've found that the chunk for the new (i,j,k) index can be
                // reused. Copy the chunk over so we can reuse it.
                if (in_x_bounds && in_y_bounds && in_z_bounds) {
                    chunks_helper[i][j][k] =
                        chunks[old_index_x][old_index_y][old_index_z];
                    chunks[old_index_x][old_index_y][old_index_z] = nullptr;
                }
            }
        }
    }

    // Now, iterate through and
    // 1) Destroy old chunks too far from our center
    // 2) For chunks we can reuse, copy their triangles to the new triangle
    // pool.
    //    We need to do this so we can free the triangles from deleted chunks
    //    (which will shift the triangle_starts of existing chunks).
    triangle_pool_helper.clear();

    for (int i = 0; i < TERRAIN_CHUNK_COUNT; i++) {
        for (int j = 0; j < TERRAIN_CHUNK_COUNT; j++) {
            for (int k = 0; k < TERRAIN_CHUNK_COUNT; k++) {
                // Free memory for old chunks
                if (chunks[i][j][k] != nullptr)
                    unloadChunk(i, j, k);

                // Remove unused triangles
                if (chunks_helper[i][j][k] != nullptr) {
                    const UINT start = chunks_helper[i][j][k]->triangle_start;
                    const UINT count = chunks_helper[i][j][k]->triangle_count;

                    const UINT new_start = triangle_pool_helper.size();
                    triangle_pool_helper.insert(triangle_pool_helper.end(),
                                                triangle_pool.begin() + start,
                                                triangle_pool.begin() + start +
                                                    count);

                    chunks_helper[i][j][k]->triangle_start = new_start;
                }
            }
        }
    }

    triangle_pool.clear();
    triangle_pool.insert(triangle_pool.end(), triangle_pool_helper.begin(),
                         triangle_pool_helper.end());

    // Now, finally iterate over the chunks and load new ones as necessary.
    for (int i = 0; i < TERRAIN_CHUNK_COUNT; i++) {
        for (int j = 0; j < TERRAIN_CHUNK_COUNT; j++) {
            for (int k = 0; k < TERRAIN_CHUNK_COUNT; k++) {
                // Create new chunks
                if (chunks_helper[i][j][k] == nullptr)
                    loadChunk(i, j, k, false);
            }
        }
    }

    // Copy our helper array to the actual terrain array
    memcpy(chunks, chunks_helper, sizeof(chunks));
}

// LoadChunk:
// Loads a terrain chunk, by sampling the perlin noise function, and
// generating a triangulation for the terrain chunk.
// If direct_load == true, loads into chunks directly. Otherwise, loads into
// chunks_helper
void Terrain::loadChunk(int index_x, int index_y, int index_z,
                        bool direct_load) {
    // Determine the bottom-left world coordinates of the chunk, both in
    // world coordinates and index coordinates
    const float x =
        (index_x + center_x - TERRAIN_CHUNK_EXTENT) * TERRAIN_CHUNK_SIZE;
    const float y =
        (index_y + center_y - TERRAIN_CHUNK_EXTENT) * TERRAIN_CHUNK_SIZE;
    const float z =
        (index_z + center_z - TERRAIN_CHUNK_EXTENT) * TERRAIN_CHUNK_SIZE;

    // Offset between samples
    constexpr float CHUNK_OFFSET =
        TERRAIN_CHUNK_SIZE / (TERRAIN_CHUNK_SAMPLES - 1);

    // Create my chunk and sample the noise function
    Chunk* chunk = new Chunk();

    for (int i = 0; i < TERRAIN_CHUNK_SAMPLES; i++) {
        for (int j = 0; j < TERRAIN_CHUNK_SAMPLES; j++) {
            for (int k = 0; k < TERRAIN_CHUNK_SAMPLES; k++) {
                const float sample_x = x + i * CHUNK_OFFSET;
                const float sample_y = y + j * CHUNK_OFFSET;
                const float sample_z = z + k * CHUNK_OFFSET;

                // --- NOISE SAMPLING ---
                constexpr float SURFACE = 0.375f;
                constexpr float FREQ = 0.0075f;

                const float val = noise_func.noise3D(
                    FREQ * sample_x, FREQ * sample_y, FREQ * sample_z);

                // Offset sampled value as marching cubes considers a
                // surface where 0 is.
                chunk->data[i][j][k] = val - SURFACE;
            }
        }
    }

    // Generate chunk mesh using the marching cubes algorithm
    chunk->triangle_start = triangle_pool.size();
    chunk->triangle_count = 0;

    int num_triangles;
    Triangle triangles[12];

    MarchingCube marching_cube = MarchingCube();
    for (int i = 0; i < TERRAIN_CHUNK_SAMPLES - 1; i++) {
        for (int j = 0; j < TERRAIN_CHUNK_SAMPLES - 1; j++) {
            for (int k = 0; k < TERRAIN_CHUNK_SAMPLES - 1; k++) {
                // Load the data into my marching cube
                marching_cube.updateData(
                    chunk->data[i][j][k], chunk->data[i + 1][j][k],
                    chunk->data[i + 1][j + 1][k], chunk->data[i][j + 1][k],
                    chunk->data[i][j][k + 1], chunk->data[i + 1][j][k + 1],
                    chunk->data[i + 1][j + 1][k + 1],
                    chunk->data[i][j + 1][k + 1]);

                // Generate my mesh for this cube
                num_triangles = 0;
                marching_cube.generateSurface(triangles, &num_triangles);

                // Load this data into my triangle pool and update my
                // chunk. We need to offset and scale the triangles so
                // they correspond to this chunk's world space position.
                assert(num_triangles <= 12);
                for (int tri = 0; tri < num_triangles; tri++) {
                    Triangle triangle = triangles[tri];

                    triangle.vertex(0).set(
                        (triangle.vertex(0) + Vector3(i, j, k)) * CHUNK_OFFSET +
                        Vector3(x, y, z));
                    triangle.vertex(1).set(
                        (triangle.vertex(1) + Vector3(i, j, k)) * CHUNK_OFFSET +
                        Vector3(x, y, z));
                    triangle.vertex(2).set(
                        (triangle.vertex(2) + Vector3(i, j, k)) * CHUNK_OFFSET +
                        Vector3(x, y, z));

                    triangle_pool.push_back(triangle);
                }

                chunk->triangle_count += num_triangles;
            }
        }
    }

    // Assign chunk to my specific index. Assumes that the original spot is open
    if (direct_load)
        chunks[index_x][index_y][index_z] = chunk;
    else
        chunks_helper[index_x][index_y][index_z] = chunk;
}

void Terrain::unloadChunk(int index_x, int index_y, int index_z) {
    // Delete chunk memory
    if (chunks[index_x][index_y][index_z] != nullptr)
        delete chunks[index_x][index_y][index_z];

    // Reset pointer
    chunks[index_x][index_y][index_z] = nullptr;
}

} // namespace Datamodel
} // namespace Engine