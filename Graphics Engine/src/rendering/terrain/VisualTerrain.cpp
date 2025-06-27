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
    // CLEANING MAY CAUSE THE ALLOCATION POINTERS TO CAUSE A MEMORY
    // CORRUPTION...
    if (dirty) {
        mesh_pool->cleanAndCompact(context);
    }

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

/*
static int generateTreeMeshHelper(MeshBuilder& builder,
                                  const std::vector<TreeStructure>& grammar,
                                  int index, const Vector3& position,
                                  const Vector2& rotation) {
    if (index >= grammar.size())
        return -1;

    const TreeStructure tree = grammar[index];

    switch (tree.token) {
    case TRUNK: {
        const float phi = rotation.u;
        const float theta = rotation.v;

        Vector3 direction = SphericalToEuler(1.0, theta, phi);
        const Quaternion rotation_offset =
            Quaternion::RotationAroundAxis(Vector3::PositiveX(), -PI / 2);
        direction = rotation_offset.rotationMatrix3() * direction;

        const Vector3 next_pos =
            position + direction * tree.trunk_data.trunk_length;

        builder.setColor(Color(150.f / 255.f, 75.f / 255.f, 0));
        builder.addTube(position, next_pos, tree.trunk_data.trunk_thickess, 5);
        return generateTreeMeshHelper(builder, grammar, index + 1, next_pos,
                                      rotation);
    } break;

    case BRANCH: {
        const Vector2 new_rotation =
            rotation + Vector2(tree.branch_data.branch_angle_phi,
                               tree.branch_data.branch_angle_theta);

        const int next_index = generateTreeMeshHelper(
            builder, grammar, index + 1, position, new_rotation);
        return generateTreeMeshHelper(builder, grammar, next_index, position,
                                      rotation);
    } break;

    case LEAF: {
        builder.setColor(Color::Green());

        const Vector3 random_axis =
            Vector3(1 + Random(0.f, 1.f), Random(0.f, 1.f), Random(0.f, 1.f))
                .unit();
        const float angle = Random(0, 2 * PI);

        builder.addCube(position,
                        Quaternion::RotationAroundAxis(random_axis, angle),
                        tree.leaf_data.leaf_density);
        return index + 1;
    } break;
    }

    return -1;
}

Mesh* VisualTerrain::generateTreeMesh(MeshBuilder& builder,
                                      const Vector2& location) {
    builder.reset();

    TreeGenerator gen = TreeGenerator();
    gen.generateTree();

    // Rotation stores (phi, theta), spherical angles. rho is assumed to be 1.
    const float x = location.u + terrain->getX();
    const float z = location.v + terrain->getZ();

    generateTreeMeshHelper(builder, gen.getTree(), 0,
                           Vector3(x, terrain->sampleTerrainHeight(x, z), z),
                           Vector2(0, 0));

    builder.regenerateNormals();

    return builder.generate();
}
*/

} // namespace Graphics
} // namespace Engine