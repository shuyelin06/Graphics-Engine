#include "Terrain.h"

#include <assert.h>

#include "core/ThreadPool.h"

#include "math/Compute.h"
#include "math/PerlinNoise.h"
#include "math/Triangle.h"

namespace Engine {
namespace Datamodel {
Terrain::Terrain() : Object(), Bindable<Terrain>(this) {
    surface_height = 100.f;
    center_x = center_y = center_z = INT_MAX;

    generator = std::make_unique<TerrainGenerator>();

    setName("Terrain");
    Terrain::SignalObjectCreation(this);
}

Terrain::~Terrain() = default;

void Terrain::propertyDisplay() {
#ifdef IMGUI_ENABLED
    generator->propertyDisplay();

    if (ImGui::Button("Invalidate Terrain")) {
        checkAndReloadChunks(true);
    }
#endif
}

// --- Accessors ---
float Terrain::getSurfaceHeight() const { return surface_height; }

const TerrainChunk& Terrain::getChunk(int i, int j, int k) {
    return chunks[i][j][k];
}
const TerrainChunk& Terrain::getChunk(const ChunkIndex& arr_index) {
    return getChunk(arr_index.x, arr_index.y, arr_index.z);
}

// --- Updates ---
// ReloadTerrain:
void Terrain::invalidateTerrain(float x, float y, float z) {
    // Calculate the chunk index that these x,y,z coordinates are in
    const int x_i = floor(x / TERRAIN_CHUNK_SIZE);
    const int y_i = floor(y / TERRAIN_CHUNK_SIZE);
    const int z_i = floor(z / TERRAIN_CHUNK_SIZE);

    center_x = x_i;
    center_y = y_i;
    center_z = z_i;

    checkAndReloadChunks();
}

void Terrain::seed(unsigned int new_seed) {
    generator->seedGenerator(new_seed);
    checkAndReloadChunks(true);
}

// CheckAndReloadChunks:
// Iterates over the chunks, and if their indices do not match then reloads the
// chunk.
void Terrain::checkAndReloadChunks(bool force_invalidate) {
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

                TerrainChunk& chunk = chunks[index_x][index_y][index_z];

                // If the x,y,z indices do not match, then we need to reload the
                // chunk data.
                const bool x_match = (chunk.chunk_x == chunk_x);
                const bool y_match = (chunk.chunk_y == chunk_y);
                const bool z_match = (chunk.chunk_z == chunk_z);

                if (force_invalidate || !(x_match && y_match && z_match)) {
                    const ChunkIndex world_index = {chunk_x, chunk_y, chunk_z};
                    reloadChunk(&chunk, world_index);
                }
            }
        }
    }
}

// LoadChunk:
// Loads a terrain chunk, by sampling the perlin noise function
// TODO: Biome Selection
void Terrain::reloadChunk(TerrainChunk* chunk, const ChunkIndex& world_index) {
    chunk->update_id++;

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
                chunk->data[i][j][k] = generator->sampleTerrainGenerator(
                    sample_x, sample_y, sample_z);
            }
        }
    }
}

} // namespace Datamodel
} // namespace Engine