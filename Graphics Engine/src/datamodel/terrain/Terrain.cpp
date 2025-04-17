#include "Terrain.h"

#include <assert.h>

#include "MarchingCube.h"
#include "core/ThreadPool.h"

#include "math/PerlinNoise.h"
#include "math/Triangle.h"

namespace Engine {
namespace Datamodel {
void Chunk::setDirty(bool status) { dirty.store(status); }
bool Chunk::isDirty() const { return dirty.load(); }

Terrain::Terrain() : noise_func(0) {
    center_x = center_y = center_z = 0;

    for (int i = 0; i < TERRAIN_CHUNK_COUNT; i++) {
        for (int j = 0; j < TERRAIN_CHUNK_COUNT; j++) {
            for (int k = 0; k < TERRAIN_CHUNK_COUNT; k++) {
                bvh_array[i][j][k] = BVH();

                chunks[i][j][k].dirty = true;
                scheduleTerrainReload(i, j, k);
            }
        }
    }
}

Terrain::~Terrain() = default;

// --- Accessors ---
const TLAS& Terrain::getTLAS() const { return tlas; }

int Terrain::getCenterChunkX() const { return center_x; }
int Terrain::getCenterChunkY() const { return center_y; }
int Terrain::getCenterChunkZ() const { return center_z; }

const Chunk* Terrain::getChunk(int x_i, int y_i, int z_i) const {
    return &chunks[x_i][y_i][z_i];
}

// ReloadTerrain:
void Terrain::invalidateTerrain(float x, float y, float z) {
    // Calculate the chunk index that these x,y,z coordinates are in
    const int x_i = floor(x / TERRAIN_CHUNK_SIZE);
    const int y_i = floor(y / TERRAIN_CHUNK_SIZE);
    const int z_i = floor(z / TERRAIN_CHUNK_SIZE);

    // If the center chunk index has not changed, do nothing.
    if (center_x == x_i && center_y == y_i && center_z == z_i)
        return;

    center_x = x_i;
    center_y = y_i;
    center_z = z_i;

    // Iterate through -TERRAIN_CHUNK_EXTENT to TERRAIN_CHUNK_EXTENT around this
    // chunk's indices, and find chunks that are dirty. These are chunks whose
    // indices no longer correspond to the chunk indices that need to be loaded.
    // Each chunk index in the world corresponds to exactly 1 index in the
    // array. We figure out this index by applying the modulus operator (that
    // wraps for negatives too).
    for (int i = -TERRAIN_CHUNK_EXTENT; i <= TERRAIN_CHUNK_EXTENT; i++) {
        const int chunk_x = center_x + i;
        const int index_x =
            (((chunk_x + TERRAIN_CHUNK_EXTENT) % TERRAIN_CHUNK_COUNT) +
             TERRAIN_CHUNK_COUNT) %
            TERRAIN_CHUNK_COUNT;

        for (int j = -TERRAIN_CHUNK_EXTENT; j <= TERRAIN_CHUNK_EXTENT; j++) {
            const int chunk_y = center_y + j;
            const int index_y =
                (((chunk_y + TERRAIN_CHUNK_EXTENT) % TERRAIN_CHUNK_COUNT) +
                 TERRAIN_CHUNK_COUNT) %
                TERRAIN_CHUNK_COUNT;

            for (int k = -TERRAIN_CHUNK_EXTENT; k <= TERRAIN_CHUNK_EXTENT;
                 k++) {
                const int chunk_z = center_z + k;
                const int index_z =
                    (((chunk_z + TERRAIN_CHUNK_EXTENT) % TERRAIN_CHUNK_COUNT) +
                     TERRAIN_CHUNK_COUNT) %
                    TERRAIN_CHUNK_COUNT;

                // If the chunk is dirty, don't do anything.
                Chunk& chunk = chunks[index_x][index_y][index_z];

                if (chunk.isDirty())
                    continue;

                // Otherwise, check if the x,y,z indices match.
                // If they do not, then mark the chunk as dirty so it can be
                // reloaded.
                const bool x_match = (chunk.chunk_x == chunk_x);
                const bool y_match = (chunk.chunk_y == chunk_y);
                const bool z_match = (chunk.chunk_z == chunk_z);

                if (!(x_match && y_match && z_match)) {
                    chunk.setDirty(true);
                    scheduleTerrainReload(index_x, index_y, index_z);
                }
            }
        }
    }

    // Finally, update our TLAS
    tlas.reset();
    for (int i = 0; i < TERRAIN_CHUNK_COUNT; i++) {
        for (int j = 0; j < TERRAIN_CHUNK_COUNT; j++) {
            for (int k = 0; k < TERRAIN_CHUNK_COUNT; k++) {
                if (chunks[i][j][k].dirty)
                    continue;

                tlas.addTLASNode(&bvh_array[i][j][k], Matrix4::Identity());
            }
        }
    }
    tlas.build();
}

// ScheduleChunkReload:
// Schedules a job on the ThreadPool to reload the chunk at specified indices
void Terrain::scheduleTerrainReload(int index_x, int index_y, int index_z) {
    ThreadPool::GetThreadPool()->scheduleJob([this, index_x, index_y, index_z] {
        this->loadChunk(index_x, index_y, index_z);
    });
}

// LoadChunk:
// Loads a terrain chunk, by sampling the perlin noise function, and
// generating a triangulation for the terrain chunk.
// If direct_load == true, loads into chunks directly. Otherwise, loads into
// chunks_helper
void Terrain::loadChunk(int index_x, int index_y, int index_z) {
    Chunk* chunk = &chunks[index_x][index_y][index_z];
    chunk->chunk_x = index_x + center_x - TERRAIN_CHUNK_EXTENT;
    chunk->chunk_y = index_y + center_y - TERRAIN_CHUNK_EXTENT;
    chunk->chunk_z = index_z + center_z - TERRAIN_CHUNK_EXTENT;

    // Determine the bottom-left world coordinates of the chunk, both in
    // world coordinates and index coordinates
    const float x = chunk->chunk_x * TERRAIN_CHUNK_SIZE;
    const float y = chunk->chunk_y * TERRAIN_CHUNK_SIZE;
    const float z = chunk->chunk_z * TERRAIN_CHUNK_SIZE;

    // Offset between samples
    constexpr float CHUNK_OFFSET =
        TERRAIN_CHUNK_SIZE / (TERRAIN_CHUNK_SAMPLES - 1);

    // Create my chunk and sample the noise function
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
    BVH& chunk_bvh = bvh_array[index_x][index_y][index_z];
    chunk_bvh.reset();

    chunk->triangles.clear();

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

                    chunk->triangles.push_back(triangle);
                    chunk_bvh.addBVHTriangle(triangle, nullptr);
                }
            }
        }
    }

    // Generate my chunk's BVH
    chunk_bvh.build();

    chunk->setDirty(false);
}

void Terrain::unloadChunk(int index_x, int index_y, int index_z) {
    // Set chunk dirty to true.
    chunks[index_x][index_y][index_z].dirty = true;
}

} // namespace Datamodel
} // namespace Engine