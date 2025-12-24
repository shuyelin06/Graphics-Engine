#include "VisualTerrain.h"

#include <assert.h>
#include <unordered_map>

#include "core/ThreadPool.h"

#include "math/AABB.h"
#include "math/Vector3.h"

#include "rendering/ImGui.h"

#include "../util/CPUTimer.h"
#include "datamodel/terrain/TerrainGenerator.h"

constexpr int MAX_CHUNK_JOBS = 16;

constexpr int OCTREE_MAX_DEPTH = 8;
constexpr float TERRAIN_VOXEL_SIZE = 25.f;

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
    inactive_jobs.reserve(MAX_CHUNK_JOBS);

    // Initialize my water surface mesh.
    water_surface = new WaterSurface();
    water_surface->generateSurfaceMesh(
        resource_manager.createMeshBuilder(MeshPoolType_Default), context, 15);
    water_surface->generateWaveConfig(14);

    surface_level = 0.f;

    octree = std::make_unique<Octree>(10, TERRAIN_VOXEL_SIZE);
}
VisualTerrain::~VisualTerrain() = default;

// GenerateTerrainMesh:
// Generates the mesh for the terrain
void VisualTerrain::updateAndUploadTerrainData(ID3D11DeviceContext* context,
                                               RenderPassTerrain& pass_terrain,
                                               const Vector3& camera_pos) {
    ICPUTimer cpu_timer = CPUTimer::TrackCPUTime("Terrain Update");

    // 1) Update the Octree based on the camera
    // TODO: Move Updater
    OctreeUpdater updater = octree->getUpdater();
    updater.updatePointOfFocus(camera_pos);

    static float sizes[OCTREE_MAX_DEPTH - 1] = {100.f, 100.f, 100.f};
    float accumulated_size = 0;
    for (int i = 0; i < OCTREE_MAX_DEPTH - 1; i++) {
        accumulated_size += sizes[i];
        // Guarantee valid input for now
        updater.updateLODDistance(i, accumulated_size);
    }

    octree->update(updater);

    // Optional Draw:
    // octree->debugDrawLeaves();

    // 2) Iterate through my existing jobs to see if any have finished. If they
    // have, load their meshes into the mesh pool.
    inactive_jobs.clear();
    bool mesh_pool_dirty = false;

    for (int i = 0; i < jobs.size(); i++) {
        auto& job = jobs[i];
        if (job->async_lock.try_lock()) {
            if (job->status == ChunkBuilderJob::JobStatus::Done) {
                const unsigned int chunk_id = job->chunk_id;

                // If the chunk id is not "active", then the chunk was unloaded
                // while the job was generating the mesh. Don't upload, and set
                // the job to inactive to be used again.
                if (octree->isNodeLeaf(chunk_id)) {
                    std::shared_ptr<Mesh> mesh =
                        job->builder.generateMesh(context);
                    assert(terrain_meshes[chunk_id] == nullptr);
                    terrain_meshes[chunk_id] = std::move(mesh);

                    mesh_pool_dirty = true;
                }

                job->status = ChunkBuilderJob::JobStatus::Inactive;
            }

            // There is a chance the async thread has not started yet at all, in
            // which case the status would be "active". So, we only want to kick
            // off a job if the status is inactive.
            if (job->status == ChunkBuilderJob::JobStatus::Inactive) {
                inactive_jobs.push_back(i);
            }

            job->async_lock.unlock();
        }
    }

    // 2) If we have inactive jobs, we will find the next highest priority
    // chunks to load the meshes of.
    const int num_inactive_jobs = inactive_jobs.size();

    if (num_inactive_jobs > 0) {
        assert(dirty_chunks.size() == 0);

        for (const auto& pair : octree->getNodeMap()) {
            const OctreeNodeID nodeID = pair.first;
            const OctreeNode* node = pair.second;

            // Skip if the chunk is in the terrain_meshes map. That means either
            // an existing job is loading the mesh, or it is already loaded.
            if (!node->isLeaf() || terrain_meshes.contains(nodeID))
                continue;

            // Compute the priority, and add to dirty chunks queue.
            assert(node != nullptr);
            const DirtyChunk dirty_chunk = {nodeID,
                                            -computeChunkPriority(*node)};

            dirty_chunks.push(dirty_chunk);

            // Push to the heap, and make sure the heap does not
            // have a size greater than the max number of jobs.

            // Note: We swap the sign of the priority, because
            // dirty_chunks is a max heap and we want a min heap
            // (so we can maintain the highest XX priorities).
            if (dirty_chunks.size() > num_inactive_jobs)
                dirty_chunks.pop();
        }

        // For any inactive jobs, we kick off another task with the highest
        // priority dirty chunk.
        assert(inactive_jobs.size() >= dirty_chunks.size());

        int next_inactive_job = 0;
        while (!dirty_chunks.empty()) {
            auto& job = jobs[inactive_jobs[next_inactive_job++]];
            job->async_lock.lock();

            const DirtyChunk dirty_chunk = dirty_chunks.top();
            dirty_chunks.pop();

            const OctreeNode* node = octree->getNode(dirty_chunk.chunk_id);
            assert(node != nullptr);
            TerrainGenerator& generator = terrain->getGenerator();

            // Load data
            terrain_meshes[dirty_chunk.chunk_id] = nullptr;
            loadChunkJobData(*job, generator, *node);
            job->status = ChunkBuilderJob::JobStatus::Active;

            // Kick off thread
            ChunkBuilderJob* job_ptr = job.get();
            ThreadPool::GetThreadPool()->scheduleJob(
                [job_ptr] { job_ptr->buildChunkMesh(); });
            job->async_lock.unlock();
        }
    }

    // 3) Finally, go through our terrain meshes. If they are not in the list of
    // active chunks, free.
    auto it = terrain_meshes.begin();
    while (it != terrain_meshes.end()) {
        const auto& pair = *it;

        if (!octree->isNodeLeaf(pair.first)) {
            mesh_pool_dirty = true;
            it = terrain_meshes.erase(it);
        } else
            ++it;
    }

    // If anything wrote to our mesh pool, then we will upload the data to
    // GPU again.
    // TODO: We could throttle this so we only clean and compact every XX
    // frames..
    if (mesh_pool_dirty) {
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
        pass_terrain.sb_indices.uploadData(context,
                                           mesh_pool->cpu_ibuffer.get(),
                                           mesh_pool->triangle_size * 3);
        pass_terrain.sb_positions.uploadData(
            context, mesh_pool->cpu_vbuffers[POSITION].get(),
            mesh_pool->vertex_size);
        pass_terrain.sb_normals.uploadData(
            context, mesh_pool->cpu_vbuffers[NORMAL].get(),
            mesh_pool->vertex_size);
    }
}

void VisualTerrain::loadChunkJobData(ChunkBuilderJob& job,
                                     const TerrainGenerator& generator,
                                     const OctreeNode& chunk) {
    assert(chunk.isLeaf());

    job.vertex_map.clear();
    job.border_triangles.clear();

    job.builder.reset();
    job.builder.addLayout(POSITION);
    job.builder.addLayout(NORMAL);

    // Load Sampled Data
    job.chunk_id = chunk.uniqueID;
    job.chunk_position =
        chunk.center - Vector3(chunk.extents, chunk.extents, chunk.extents);
    job.chunk_size = chunk.extents * 2;

    const float DISTANCE_BETWEEN_SAMPLES =
        job.chunk_size / (TERRAIN_SAMPLES_PER_CHUNK - 1);

    for (int i = 0; i < TERRAIN_SAMPLES_PER_CHUNK + 2; i++) {
        for (int j = 0; j < TERRAIN_SAMPLES_PER_CHUNK + 2; j++) {
            for (int k = 0; k < TERRAIN_SAMPLES_PER_CHUNK + 2; k++) {
                const float sample_x =
                    job.chunk_position.x + (i - 1) * DISTANCE_BETWEEN_SAMPLES;
                const float sample_y =
                    job.chunk_position.y + (j - 1) * DISTANCE_BETWEEN_SAMPLES;
                const float sample_z =
                    job.chunk_position.z + (k - 1) * DISTANCE_BETWEEN_SAMPLES;

                job.data[i][j][k] = generator.sampleTerrainGenerator(
                    sample_x, sample_y, sample_z);
            }
        }
    }
}

float VisualTerrain::computeChunkPriority(const OctreeNode& chunk) {
    float distance =
        (Vector3(chunk.center.x, chunk.center.y, chunk.center.z)).magnitude();
    return 1 / (1 + distance);
}

// --- Accessors ---
float VisualTerrain::getSurfaceLevel() const { return surface_level; }

// Returns the water surface mesh
const WaterSurface* VisualTerrain::getWaterSurface() const {
    return water_surface;
}

// ImGui
void VisualTerrain::imGui() {
#if defined(IMGUI_ENABLED)

#endif
}

} // namespace Graphics
} // namespace Engine