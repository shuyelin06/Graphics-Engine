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
    // Initialize our buffer pool. This will consolidate our mesh data
    // into a few vertex / index buffers, to dramatically reduce the number of
    // draw calls we make.
    // Empirical testing has shown that 300,000 vertices, 200,000 indices is
    // enough.
    output_mesh = new BufferPool(device, 300000, 200000);

    // Set all current chunk_meshes to null, and initialize my terrain
    // callbacks.
    for (int i = 0; i < TERRAIN_CHUNK_COUNT; i++) {
        for (int j = 0; j < TERRAIN_CHUNK_COUNT; j++) {
            for (int k = 0; k < TERRAIN_CHUNK_COUNT; k++) {
                allocations[i][j][k] = nullptr;

                callbacks[i][j][k].initialize(device);
                terrain->registerTerrainCallback(i, j, k, &callbacks[i][j][k]);
            }
        }
    }

    // Initialize my water surface mesh.
    water_surface = new WaterSurface();
    water_surface->generateSurfaceMesh(device, 450.f, 10);
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
                    dirty = true;

                    if (allocations[i][j][k] != nullptr) {
                        allocations[i][j][k]->valid = false;
                        allocations[i][j][k] = nullptr;
                    }
                    allocations[i][j][k] = callback.loadMesh(*output_mesh);
                }
            }
        }
    }

    // CLEANING MAY CAUSE THE ALLOCATION POINTERS TO CAUSE A MEMORY
    // CORRUPTION...
    if (dirty) {
        output_mesh->cleanAndCompact();
        output_mesh->updateGPUResources(context);
    }
}

// --- Accessors ---
float VisualTerrain::getSurfaceLevel() const { return surface_level; }

// Returns the chunk meshes
BufferPool* VisualTerrain::getMesh() { return output_mesh; }

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