#include "VisualTerrain.h"

#include <assert.h>
#include <unordered_map>

#if defined(_DEBUG)
#include "../ImGui.h"
#endif

namespace Engine {
using namespace Datamodel;

namespace Graphics {
VisualTerrain::VisualTerrain(Terrain* _terrain, ID3D11DeviceContext* context,
                             ResourceManager& resource_manager)
    : terrain(_terrain), callbacks() {
    mesh_pool = resource_manager.getMeshPool(MeshPoolType_Terrain);

    // Set all current chunk_meshes to null, and initialize my terrain
    // callbacks.
    for (int i = 0; i < TERRAIN_CHUNK_COUNT; i++) {
        for (int j = 0; j < TERRAIN_CHUNK_COUNT; j++) {
            for (int k = 0; k < TERRAIN_CHUNK_COUNT; k++) {
                meshes[i][j][k] = nullptr;

                callbacks[i][j][k] =
                    std::make_unique<VisualTerrainCallback>(mesh_pool);
                terrain->registerTerrainCallback(i, j, k,
                                                 callbacks[i][j][k].get());
            }
        }
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
    // Iterate through my callbacks. If they have a mesh, overwrite what we
    // currently have.
    bool dirty = false;

    for (int i = 0; i < TERRAIN_CHUNK_COUNT; i++) {
        for (int j = 0; j < TERRAIN_CHUNK_COUNT; j++) {
            for (int k = 0; k < TERRAIN_CHUNK_COUNT; k++) {
                VisualTerrainCallback& callback = *callbacks[i][j][k];

                if (callback.isDirty()) {
                    dirty = true;
                    meshes[i][j][k] = callback.loadMesh(context);
                }
            }
        }
    }

    // Clean and compact my mesh pool
    if (dirty)
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