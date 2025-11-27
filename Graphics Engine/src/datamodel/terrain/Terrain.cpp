#include "Terrain.h"

#include <assert.h>

#include "MarchingCube.h"
#include "core/ThreadPool.h"

#include "math/Compute.h"
#include "math/PerlinNoise.h"
#include "math/Triangle.h"

namespace Engine {
namespace Datamodel {
Terrain::Terrain() : Object(), Bindable<Terrain>(this), noise_func(0) {
    surface_height = 100.f;
    center_x = center_y = center_z = INT_MAX;
    cur_seed = 0;

    setName("Terrain");
    Terrain::SignalObjectCreation(this);
}

Terrain::~Terrain() = default;

void Terrain::propertyDisplay() {
#ifdef IMGUI_ENABLED
    static int terrain_seed = 0;
    ImGui::SliderInt("Seed", &terrain_seed, 0, 0xFFF);
    if (ImGui::Button("Invalidate Terrain")) {
        seed(terrain_seed);
    }
#endif
}

// --- Initializers ---
void Terrain::registerTerrainCallback(int i, int j, int k,
                                      TerrainCallback* callback) {
    chunks[i][j][k].callbacks.push_back(callback);
}

// --- Accessors ---
float Terrain::getSurfaceHeight() const { return surface_height; }

// --- Updates ---
// ReloadTerrain:
void Terrain::invalidateTerrain(float x, float y, float z) {
    // Calculate the chunk index that these x,y,z coordinates are in
    const int x_i = floor(x / TERRAIN_CHUNK_SIZE);
    const int y_i = floor(y / TERRAIN_CHUNK_SIZE);
    const int z_i = floor(z / TERRAIN_CHUNK_SIZE);

    // If the center chunk index has not changed too much, do nothing.
    if (abs(center_x - x_i) <= 1 && abs(center_y - y_i) <= 1 &&
        abs(center_z - z_i) <= 1)
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
        const int index_x = Modulus(chunk_x, TERRAIN_CHUNK_COUNT);

        for (int j = -TERRAIN_CHUNK_EXTENT; j <= TERRAIN_CHUNK_EXTENT; j++) {
            const int chunk_y = center_y + j;
            const int index_y = Modulus(chunk_y, TERRAIN_CHUNK_COUNT);

            for (int k = -TERRAIN_CHUNK_EXTENT; k <= TERRAIN_CHUNK_EXTENT;
                 k++) {
                const int chunk_z = center_z + k;
                const int index_z = Modulus(chunk_z, TERRAIN_CHUNK_COUNT);

                // If the chunk is dirty, don't do anything. We know its dirty
                // if we're unable to lock-- as this means a thread is updating
                // the chunk data.
                TerrainChunk& chunk = chunks[index_x][index_y][index_z];
                std::unique_lock<std::mutex> lock(chunk.mutex,
                                                  std::try_to_lock);

                if (lock.owns_lock()) {
                    // Otherwise, check if the x,y,z indices match.
                    // If they do not, then mark the chunk as dirty so it can be
                    // reloaded.
                    const bool x_match = (chunk.chunk_x == chunk_x);
                    const bool y_match = (chunk.chunk_y == chunk_y);
                    const bool z_match = (chunk.chunk_z == chunk_z);

                    if (!(x_match && y_match && z_match)) {
                        const ChunkIndex local_index = {index_x, index_y,
                                                        index_z};
                        const ChunkIndex world_index = {chunk_x, chunk_y,
                                                        chunk_z};

                        scheduleTerrainReload(local_index, world_index);
                    }
                }
            }
        }
    }
}

void Terrain::seed(unsigned int new_seed) {
    if (cur_seed != new_seed) {
        cur_seed = new_seed;
        noise_func.seed(cur_seed);
        forceInvalidateAll();
    }
}

void Terrain::forceInvalidateAll() {
    // Iterate through -TERRAIN_CHUNK_EXTENT to TERRAIN_CHUNK_EXTENT around this
    // chunk's indices, and find chunks that are dirty. These are chunks whose
    // indices no longer correspond to the chunk indices that need to be loaded.
    // Each chunk index in the world corresponds to exactly 1 index in the
    // array. We figure out this index by applying the modulus operator (that
    // wraps for negatives too).
    for (int i = -TERRAIN_CHUNK_EXTENT; i <= TERRAIN_CHUNK_EXTENT; i++) {
        const int chunk_x = center_x + i;
        const int index_x = Modulus(chunk_x, TERRAIN_CHUNK_COUNT);

        for (int j = -TERRAIN_CHUNK_EXTENT; j <= TERRAIN_CHUNK_EXTENT; j++) {
            const int chunk_y = center_y + j;
            const int index_y = Modulus(chunk_y, TERRAIN_CHUNK_COUNT);

            for (int k = -TERRAIN_CHUNK_EXTENT; k <= TERRAIN_CHUNK_EXTENT;
                 k++) {
                const int chunk_z = center_z + k;
                const int index_z = Modulus(chunk_z, TERRAIN_CHUNK_COUNT);

                // If the chunk is dirty, don't do anything. We know its dirty
                // if we're unable to lock-- as this means a thread is updating
                // the chunk data.
                TerrainChunk& chunk = chunks[index_x][index_y][index_z];
                std::unique_lock<std::mutex> lock(chunk.mutex,
                                                  std::try_to_lock);

                if (lock.owns_lock()) {
                    const ChunkIndex local_index = {index_x, index_y, index_z};
                    const ChunkIndex world_index = {chunk_x, chunk_y, chunk_z};

                    scheduleTerrainReload(local_index, world_index);
                }
            }
        }
    }
}

// ScheduleChunkReload:
// Schedules a job on the ThreadPool to reload the chunk at specified indices
void Terrain::scheduleTerrainReload(const ChunkIndex& local_index,
                                    const ChunkIndex& world_index) {
    ThreadPool::GetThreadPool()->scheduleJob([this, local_index, world_index] {
        this->reloadChunk(local_index, world_index);
    });
}

// LoadChunk:
// Loads a terrain chunk, by sampling the perlin noise function, and
// generating a triangulation for the terrain chunk.
// If direct_load == true, loads into chunks directly. Otherwise, loads into
// chunks_helper
void Terrain::reloadChunk(const ChunkIndex local_index,
                          const ChunkIndex world_index) {
    // Lock the chunk for writing.
    TerrainChunk* chunk = &chunks[local_index.x][local_index.y][local_index.z];
    std::unique_lock<std::mutex> lock(chunk->mutex);

    chunk->chunk_x = world_index.x;
    chunk->chunk_y = world_index.y;
    chunk->chunk_z = world_index.z;

    // Determine the bottom-left world coordinates of the chunk, both in
    // world coordinates and index coordinates
    const float x = chunk->chunk_x * TERRAIN_CHUNK_SIZE;
    const float y = chunk->chunk_y * TERRAIN_CHUNK_SIZE;
    const float z = chunk->chunk_z * TERRAIN_CHUNK_SIZE;

    // Offset between samples
    constexpr float CHUNK_OFFSET =
        TERRAIN_CHUNK_SIZE / (TERRAIN_CHUNK_SAMPLES - 1);

    // Create my chunk and sample the noise function. We include samples
    // slightly outside of the chunk too so that our generated normals are
    // smooth.
    for (int i = 0; i < TERRAIN_CHUNK_SAMPLES + 2; i++) {
        for (int j = 0; j < TERRAIN_CHUNK_SAMPLES + 2; j++) {
            for (int k = 0; k < TERRAIN_CHUNK_SAMPLES + 2; k++) {
                const float sample_x = x + (i - 1) * CHUNK_OFFSET;
                const float sample_y = y + (j - 1) * CHUNK_OFFSET;
                const float sample_z = z + (k - 1) * CHUNK_OFFSET;

                // Sample our noise function
                constexpr float SURFACE = 0.375f;
                constexpr float FREQ = 0.0075f;
                float val = noise_func.noise3D(FREQ * sample_x, FREQ * sample_y,
                                               FREQ * sample_z);

                // Fade our noise to 0 depending on how close we are to the
                // water line
                constexpr float FADE_RATE = 0.0075f;
                if (sample_y >= TERRAIN_FADE_LINE) {
                    val += (sample_y - TERRAIN_FADE_LINE) * FADE_RATE;
                }
                val = Clamp(val, 0.0f, 1.0f);

                // Offset sampled value as marching cubes considers a
                // surface where 0 is.
                chunk->data[i][j][k] = val - SURFACE;
            }
        }
    }

    // Generate chunk mesh using the marching cubes algorithm
    chunk->border_triangles.clear();
    chunk->triangles.clear();

    int num_triangles;
    Triangle triangles[12];

    MarchingCube marching_cube = MarchingCube();
    for (int i = 0; i < TERRAIN_CHUNK_SAMPLES + 2 - 1; i++) {
        for (int j = 0; j < TERRAIN_CHUNK_SAMPLES + 2 - 1; j++) {
            for (int k = 0; k < TERRAIN_CHUNK_SAMPLES + 2 - 1; k++) {
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

                    // Border triangles
                    if (i == 0 || j == 0 || k == 0 ||
                        i == TERRAIN_CHUNK_SAMPLES ||
                        j == TERRAIN_CHUNK_SAMPLES ||
                        k == TERRAIN_CHUNK_SAMPLES) {
                        chunk->border_triangles.push_back(triangle);
                    }
                    // Triangles in the chunk
                    else {
                        chunk->triangles.push_back(triangle);
                    }
                }
            }
        }
    }

    // Call the chunk callback functions
    for (TerrainCallback* callback : chunk->callbacks)
        callback->reloadTerrainData(chunk);
}

} // namespace Datamodel
} // namespace Engine