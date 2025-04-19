#include "PhysicsTerrain.h"

namespace Engine {
namespace Physics {
PhysicsTerrainCallback::PhysicsTerrainCallback() = default;

BVH* PhysicsTerrainCallback::extractBVH() {
    std::unique_lock<std::mutex> lock(mutex);
    BVH* bvh = output_bvh;
    output_bvh = nullptr;

    dirty = false;

    return bvh;
}
bool PhysicsTerrainCallback::isDirty() {
    std::unique_lock<std::mutex> lock(mutex);
    return dirty;
}

void PhysicsTerrainCallback::reloadTerrainData(const TerrainChunk* chunk_data) {
    BVH* new_bvh = nullptr;

    if (chunk_data->triangles.size() > 0) {
        // Build a new BVH if there are triangles.
        new_bvh = new BVH();

        for (const Triangle& triangle : chunk_data->triangles)
            new_bvh->addBVHTriangle(triangle, nullptr);

        new_bvh->build();
    }

    // Save this BVH
    {
        std::unique_lock<std::mutex> lock(mutex);
        dirty = true;

        if (output_bvh != nullptr)
            delete output_bvh;
        output_bvh = new_bvh;
    }
}

PhysicsTerrain::PhysicsTerrain(Terrain* _terrain)
    : terrain(_terrain), tlas(), chunk_bvhs(), callbacks() {
    for (int i = 0; i < TERRAIN_CHUNK_COUNT; i++) {
        for (int j = 0; j < TERRAIN_CHUNK_COUNT; j++) {
            for (int k = 0; k < TERRAIN_CHUNK_COUNT; k++) {
                terrain->registerTerrainCallback(i, j, k, &callbacks[i][j][k]);
            }
        }
    }
}

void PhysicsTerrain::pullTerrainBVHs() {
    // Iterate through my callbacks. If they have a mesh, overwrite what we
    // currently have.
    for (int i = 0; i < TERRAIN_CHUNK_COUNT; i++) {
        for (int j = 0; j < TERRAIN_CHUNK_COUNT; j++) {
            for (int k = 0; k < TERRAIN_CHUNK_COUNT; k++) {
                PhysicsTerrainCallback& callback = callbacks[i][j][k];

                if (callback.isDirty()) {
                    if (chunk_bvhs[i][j][k] != nullptr)
                        delete chunk_bvhs[i][j][k];
                    chunk_bvhs[i][j][k] = callback.extractBVH();
                }
            }
        }
    }

    // Add to our TLAS
    // TODO: THIS BUILD IS SUPER SLOW. NEEDS TO BE REAL TIME
    /*tlas.reset();

    for (int i = 0; i < TERRAIN_CHUNK_COUNT; i++) {
        for (int j = 0; j < TERRAIN_CHUNK_COUNT; j++) {
            for (int k = 0; k < TERRAIN_CHUNK_COUNT; k++) {
                if (chunk_bvhs[i][j][k] != nullptr)
                    tlas.addTLASNode(chunk_bvhs[i][j][k], Matrix4::Identity());
            }
        }
    }

    tlas.build();*/
}

// GetTerrainTLS();
TLAS PhysicsTerrain::getTerrainTLS() { return tlas; }

} // namespace Physics
} // namespace Engine