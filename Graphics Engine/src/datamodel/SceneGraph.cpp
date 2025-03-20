#include "SceneGraph.h"

#include <assert.h>

namespace Engine {
namespace Datamodel {

Scene::Scene() : objects(), terrain(), terrain_helper() {
    noise_func = new PerlinNoise(0);

    center_chunk_x = INT_MIN, center_chunk_z = INT_MIN;
}
Scene::~Scene() {
    for (Object* object : objects)
        delete object;
}

// GetObjects:
// Returns the vector of objects in the SceneGraph
const std::vector<Object*>& Scene::getObjects() { return objects; }

// NewObject:
// Creates a new object in the scene.
Object& Scene::createObject() {
    Object* object = new Object();
    objects.push_back(object);
    return *object;
}

// --- Terrain Handling ---
TerrainChunk* Scene::getTerrainChunk(int x_index, int z_index) const {
    return terrain[x_index][z_index];
}

float Scene::sampleTerrainHeight(float x, float z) const {
    // Calculate the chunk index for these coordinates
    const int x_index =
        floor(x / HEIGHT_MAP_XZ_SIZE) - center_chunk_x + TERRAIN_CHUNK_EXTENT;
    const int z_index =
        floor(z / HEIGHT_MAP_XZ_SIZE) - center_chunk_z + TERRAIN_CHUNK_EXTENT;

    if (x_index < 0 || TERRAIN_NUM_CHUNKS <= x_index)
        return FLT_MIN;
    if (z_index < 0 || TERRAIN_NUM_CHUNKS <= z_index)
        return FLT_MIN;

    const TerrainChunk* chunk = terrain[x_index][z_index];

    if (chunk == nullptr)
        return FLT_MIN;

    return chunk->sampleTerrainHeight(x, z);
}

// SeedTerrain:
// Seeds the Perlin Noise function for terraing generation
void Scene::seedTerrain(unsigned int seed) {
    if (noise_func != nullptr)
        delete noise_func;
    noise_func = new PerlinNoise(seed);
}

// ReloadTerrain:
// Force reload terrain chunks .
void Scene::reloadTerrainChunk(int x_index, int z_index) {
    // Free the existing terrain chunk if it exists
    if (terrain[x_index][z_index] != nullptr)
        delete terrain[x_index][z_index];

    // Calculate the chunk's world position and generate the chunk
    const float chunk_x =
        (x_index - TERRAIN_CHUNK_EXTENT + center_chunk_x) * HEIGHT_MAP_XZ_SIZE;
    const float chunk_z =
        (z_index - TERRAIN_CHUNK_EXTENT + center_chunk_z) * HEIGHT_MAP_XZ_SIZE;

    TerrainChunk* chunk = new TerrainChunk(chunk_x, chunk_z, noise_func);
    terrain_helper[x_index][z_index] = chunk;

    new_chunks.push_back(chunk);
}

void Scene::reloadTerrain() {
    // Iterate through and replace every terrain chunk with a new=
    // chunk
    for (int i = 0; i < TERRAIN_NUM_CHUNKS; i++)
        for (int j = 0; j < TERRAIN_NUM_CHUNKS; j++)
            reloadTerrainChunk(i, j);
}

// --- Scene Updating ---
// UpdateSceneCenter:
// Updates the scene center and loads / unloads chunks based on this center.
void Scene::updateTerrainChunks(float new_x, float new_z) {
    new_chunks.clear();

    // Calculate the chunk these x,y coordinates are in.
    const int new_x_chunk = floor(new_x / HEIGHT_MAP_XZ_SIZE);
    const int new_z_chunk = floor(new_z / HEIGHT_MAP_XZ_SIZE);

    if (center_chunk_x == new_x_chunk && center_chunk_z == new_z_chunk)
        return;

    // Find the x,z coordinates of the chunk that the center is located in.
    const int old_center_chunk_x = center_chunk_x;
    const int old_center_chunk_z = center_chunk_z;

    center_chunk_x = new_x_chunk;
    center_chunk_z = new_z_chunk;

    std::memset(terrain_helper, 0, sizeof(terrain_helper));

    // We have a set of chunks around our old center. After moving our center,
    // we have a new set of chunks, but we don't want to regenerate chunks that
    // are the same. We will iterate through all chunks around our new center
    // and pull them from the old center chunks if possible.
    for (int i = 0; i < TERRAIN_NUM_CHUNKS; i++) {
        for (int j = 0; j < TERRAIN_NUM_CHUNKS; j++) {
            // Find our chunk index on x,z
            const int old_index_x = i + center_chunk_x - old_center_chunk_x;
            const int old_index_z = j + center_chunk_z - old_center_chunk_z;

            if (0 <= old_index_x && old_index_x < TERRAIN_NUM_CHUNKS) {
                if (0 <= old_index_z && old_index_z < TERRAIN_NUM_CHUNKS) {
                    terrain_helper[i][j] = terrain[old_index_x][old_index_z];
                    terrain[old_index_x][old_index_z] = nullptr;
                }
            }
        }
    }

    // Now, iterate through and
    // 1) Create new chunks that need to be created
    // 2) Destroy old chunks too far from our center
    for (int i = 0; i < TERRAIN_NUM_CHUNKS; i++) {
        for (int j = 0; j < TERRAIN_NUM_CHUNKS; j++) {
            // Free memory for old chunks
            if (terrain[i][j] != nullptr)
                delete terrain[i][j];

            // Create new chunks
            if (terrain_helper[i][j] == nullptr) {
                const float chunk_x =
                    (i - TERRAIN_CHUNK_EXTENT + center_chunk_x) *
                    HEIGHT_MAP_XZ_SIZE;
                const float chunk_z =
                    (j - TERRAIN_CHUNK_EXTENT + center_chunk_z) *
                    HEIGHT_MAP_XZ_SIZE;

                TerrainChunk* chunk =
                    new TerrainChunk(chunk_x, chunk_z, noise_func);
                terrain_helper[i][j] = chunk;

                new_chunks.push_back(chunk);
            }
        }
    }

    // Copy our helper array to the actual terrain array
    memcpy(terrain, terrain_helper, sizeof(terrain));
}

// GetNewChunks:
// Return the newly created terrain chunks for binding
const std::vector<TerrainChunk*>& Scene::getNewChunks() const {
    return new_chunks;
}

// UpdateAndRenderObjects:
// Update and cache object transforms in the SceneGraph, and submit
// render requests for each.
void Scene::updateObjects() {
    Matrix4 identity = Matrix4::Identity();

    for (Object* object : objects)
        updateObjectsHelper(object, identity);
}

void Scene::updateObjectsHelper(Object* object, const Matrix4& m_parent) {
    assert(object != nullptr);

    const Matrix4 m_local = object->updateLocalMatrix(m_parent);
    for (Object* child : object->getChildren())
        updateObjectsHelper(child, m_local);
}

} // namespace Datamodel
} // namespace Engine