#include "VisualTerrain.h"

#include <assert.h>
#include <unordered_map>

#if defined(_DEBUG)
#include "../ImGui.h"
#endif

namespace Engine {
using namespace Datamodel;

namespace Graphics {
VisualTerrain::VisualTerrain(Terrain* _terrain, ID3D11Device* device)
    : terrain(_terrain), callbacks() {
    // Initialize our buffer pools and structured buffers. This will consolidate
    // our mesh data to dramatically reduce
    // the number of draw calls we make. Empirical testing has shown that
    // 300,000 vertices, 200,000 indices is enough.
    uint16_t layout = (1 << POSITION) | (1 << NORMAL);
    mesh_pool = new MeshPool(device, layout, 200000, 300000);

    sb_descriptors.initialize(device, TERRAIN_CHUNK_COUNT *
                                          TERRAIN_CHUNK_COUNT *
                                          TERRAIN_CHUNK_COUNT);
    sb_indices.initialize(device, 200000 * 3);
    sb_positions.initialize(device, 300000);
    sb_normals.initialize(device, 300000);

    // Set all current chunk_meshes to null, and initialize my terrain
    // callbacks.
    for (int i = 0; i < TERRAIN_CHUNK_COUNT; i++) {
        for (int j = 0; j < TERRAIN_CHUNK_COUNT; j++) {
            for (int k = 0; k < TERRAIN_CHUNK_COUNT; k++) {
                meshes[i][j][k] = nullptr;

                callbacks[i][j][k].initialize();
                terrain->registerTerrainCallback(i, j, k, &callbacks[i][j][k]);
            }
        }
    }

    // Initialize my water surface mesh.
    water_surface = new WaterSurface();
    water_surface->generateSurfaceMesh(device, 15);
    water_surface->generateWaveConfig(14);

    surface_level = 100.f;
}
VisualTerrain::~VisualTerrain() = default;

// GenerateTerrainMesh:
// Generates the mesh for the terrain
void VisualTerrain::pullTerrainMeshes(ID3D11DeviceContext* context) {
    // Iterate through my callbacks. If they have a mesh, overwrite what we
    // currently have.
    bool dirty = false;

    for (int i = 0; i < TERRAIN_CHUNK_COUNT; i++) {
        for (int j = 0; j < TERRAIN_CHUNK_COUNT; j++) {
            for (int k = 0; k < TERRAIN_CHUNK_COUNT; k++) {
                VisualTerrainCallback& callback = callbacks[i][j][k];

                if (callback.isDirty()) {
                    if (meshes[i][j][k] != nullptr) {
                        dirty = true;
                        delete meshes[i][j][k];
                        meshes[i][j][k] = nullptr;
                    }

                    meshes[i][j][k] = callback.loadMesh(context, mesh_pool);
                }
            }
        }
    }

    // Clean and compact my mesh pool
    if (dirty)
        mesh_pool->cleanAndCompact();

    // Upload my data to the structured buffers
    std::vector<TBChunkDescriptor> descriptors;

    num_active_chunks = mesh_pool->meshes.value().size();
    max_chunk_triangles = 0;
    for (Mesh* mesh : mesh_pool->meshes.value()) {
        TBChunkDescriptor desc;
        desc.index_start = mesh->triangle_start * 3;
        desc.index_count = mesh->num_triangles * 3;
        desc.vertex_start = mesh->vertex_start;
        desc.vertex_count = mesh->num_vertices;
        descriptors.push_back(desc);

        max_chunk_triangles = max(max_chunk_triangles, mesh->num_triangles);
    }

    sb_descriptors.uploadData(context, descriptors.data(), descriptors.size());
    sb_indices.uploadData(context, mesh_pool->cpu_ibuffer,
                          mesh_pool->triangle_size * 3);
    sb_positions.uploadData(context, mesh_pool->cpu_vbuffers[POSITION],
                            mesh_pool->vertex_size);
    sb_normals.uploadData(context, mesh_pool->cpu_vbuffers[NORMAL],
                          mesh_pool->vertex_size);
}

// --- Accessors ---
float VisualTerrain::getSurfaceLevel() const { return surface_level; }

// Returns the chunk meshes
const StructuredBuffer<TBChunkDescriptor>&
VisualTerrain::getDescriptorSB() const {
    return sb_descriptors;
}
const StructuredBuffer<unsigned int>& VisualTerrain::getIndexSB() const {
    return sb_indices;
}
const StructuredBuffer<Vector3>& VisualTerrain::getPositionSB() const {
    return sb_positions;
}
const StructuredBuffer<Vector3>& VisualTerrain::getNormalSB() const {
    return sb_normals;
}
int VisualTerrain::getActiveChunkCount() const { return num_active_chunks; }
int VisualTerrain::getMaxChunkTriangleCount() const {
    return max_chunk_triangles;
}

// Returns the water surface mesh
const WaterSurface* VisualTerrain::getWaterSurface() const {
    return water_surface;
}

} // namespace Graphics
} // namespace Engine