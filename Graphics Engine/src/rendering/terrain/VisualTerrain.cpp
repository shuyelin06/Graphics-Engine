#include "VisualTerrain.h"

#include <assert.h>
#include <unordered_map>

#if defined(_DEBUG)
#include "../ImGui.h"
#endif

#include "core/ThreadPool.h"

constexpr int MAX_CHUNK_JOBS = 16;

namespace Engine {
using namespace Datamodel;

namespace Graphics {
VisualTerrain::VisualTerrain(Terrain* _terrain, ID3D11DeviceContext* context,
                             ResourceManager& resource_manager)
    : terrain(_terrain) {
    mesh_pool = resource_manager.getMeshPool(MeshPoolType_Terrain);

    // Create MAX_CHUNK_JOBS schunk update jobs.
    for (int i = 0; i < MAX_CHUNK_JOBS; i++) {
        jobs.push_back(std::make_unique<ChunkBuilderJob>(mesh_pool));
    }

    // Initialize my water surface mesh.
    water_surface = new WaterSurface();
    water_surface->generateSurfaceMesh(
        resource_manager.createMeshBuilder(MeshPoolType_Default), context, 15);
    water_surface->generateWaveConfig(14);

    surface_level = 0.f;
}
VisualTerrain::~VisualTerrain() = default;

// GenerateTerrainMesh:
// Generates the mesh for the terrain
void VisualTerrain::updateAndUploadTerrainData(
    ID3D11DeviceContext* context, RenderPassTerrain& pass_terrain) {
    // Iterate through all chunks, and check the update IDs. If:
    // 1) The chunk update id is different than what we "know about", AND
    // 2) The chunk is not currently being processed
    // We add to dirty chunks.
    dirty_chunks.clear();

    for (int i = 0; i < TERRAIN_CHUNK_COUNT; i++) {
        for (int j = 0; j < TERRAIN_CHUNK_COUNT; j++) {
            for (int k = 0; k < TERRAIN_CHUNK_COUNT; k++) {
                const TerrainChunk& chunk = terrain->getChunk(i, j, k);
                ChunkStatus& chunk_tracker = chunk_trackers[i][j][k];

                if (chunk.update_id != chunk_tracker.chunk_update_id &&
                    !chunk_tracker.processing) {
                    dirty_chunks.push_back({i, j, k});
                }
            }
        }
    }

    // TODO: Sort dirty_chunks to figure out what jobs we want to execute.

    // Iterate through existing jobs to see what jobs have finished.
    // If any jobs have finished, upload their data to the pool and mark the job
    // as inactive.
    // For any inactive jobs, we kick off another task with the highest priority
    // dirty chunk.
    bool mesh_pool_dirty = false;
    int next_chunk = 0;

    for (auto& job : jobs) {
        if (job->async_lock.try_lock()) {
            if (job->active) {
                const auto& arr_index = job->chunk_array_index;

                auto& chunk_status =
                    chunk_trackers[arr_index.x][arr_index.y][arr_index.z];
                chunk_status.chunk_update_id = job->chunk_copy.update_id;
                chunk_status.mesh = job->builder.generateMesh(context);
                chunk_status.processing = false;

                job->active = false;

                mesh_pool_dirty = true;
            }

            if (next_chunk < dirty_chunks.size()) {
                const ChunkIndex& chunk_index = dirty_chunks[next_chunk++];

                ChunkStatus& chunk_tracker =
                    chunk_trackers[chunk_index.x][chunk_index.y][chunk_index.z];
                chunk_tracker.processing = true;

                // Load data
                const TerrainChunk& target_chunk = terrain->getChunk(
                    chunk_index.x, chunk_index.y, chunk_index.z);
                job->loadChunkData(target_chunk, chunk_index);

                // Kick off thread
                ChunkBuilderJob* job_ptr = job.get();
                ThreadPool::GetThreadPool()->scheduleJob(
                    [job_ptr] { job_ptr->buildChunkMesh(); });
            }

            job->async_lock.unlock();
        }
    }

    // Clean and compact my mesh pool
    if (mesh_pool_dirty)
        mesh_pool->cleanAndCompact();

    // Upload my data to the structured buffers
    std::vector<TBChunkDescriptor> descriptors;

    pass_terrain.num_active_chunks = mesh_pool->meshes.size();
    pass_terrain.max_chunk_triangles = 0;
    for (std::shared_ptr<Mesh>& mesh : mesh_pool->meshes) {
        TBChunkDescriptor desc;
        desc.index_start = mesh->triangle_start * 3;
        desc.index_count = mesh->num_triangles * 3;
        desc.vertex_start = mesh->vertex_start;
        desc.vertex_count = mesh->num_vertices;
        descriptors.push_back(desc);

        pass_terrain.max_chunk_triangles =
            max(pass_terrain.max_chunk_triangles, mesh->num_triangles);
    }

    pass_terrain.sb_chunks.uploadData(context, descriptors.data(),
                                      descriptors.size());
    pass_terrain.sb_indices.uploadData(context, mesh_pool->cpu_ibuffer.get(),
                                       mesh_pool->triangle_size * 3);
    pass_terrain.sb_positions.uploadData(
        context, mesh_pool->cpu_vbuffers[POSITION].get(),
        mesh_pool->vertex_size);
    pass_terrain.sb_normals.uploadData(
        context, mesh_pool->cpu_vbuffers[NORMAL].get(), mesh_pool->vertex_size);
}

// --- Accessors ---
float VisualTerrain::getSurfaceLevel() const { return surface_level; }

// Returns the water surface mesh
const WaterSurface* VisualTerrain::getWaterSurface() const {
    return water_surface;
}

} // namespace Graphics
} // namespace Engine