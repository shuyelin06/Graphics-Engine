#include "VisualTerrain.h"

#include <assert.h>
#include <unordered_map>

// Hash Function for Vector3
// Hash Function for Vector3
template <> struct std::hash<Engine::Math::Vector3> {
    std::size_t operator()(const Engine::Math::Vector3& k) const {
        // https://stackoverflow.com/questions/5928725/hashing-2d-3d-and-nd-vectors
        uint32_t hash = std::_Bit_cast<uint32_t, float>(k.x) * 73856093 ^
                        std::_Bit_cast<uint32_t, float>(k.y) * 19349663 ^
                        std::_Bit_cast<uint32_t, float>(k.z) * 83492791;

        return hash % SIZE_MAX;
    }
};

namespace Engine {
using namespace Datamodel;

namespace Graphics {
VisualTerrain::VisualTerrain(const Terrain* _terrain) : terrain(_terrain) {
    center_x = center_y = center_z = INT_MIN;

    // These size are NOT to be changed. They're coded to match the size of
    // the 3D array in Terrain, but given as a vector so the VisualSystem has an
    // easier time reading the data.
    chunk_meshes.resize(TERRAIN_CHUNK_COUNT * TERRAIN_CHUNK_COUNT *
                        TERRAIN_CHUNK_COUNT);
    chunk_meshes_helper.resize(TERRAIN_CHUNK_COUNT * TERRAIN_CHUNK_COUNT *
                               TERRAIN_CHUNK_COUNT);
}
VisualTerrain::~VisualTerrain() {
    for (int i = 0; i < chunk_meshes.size(); i++)
        if (chunk_meshes[i] != nullptr)
            delete chunk_meshes[i];
}

// GenerateTerrainMesh:
// Generates the mesh for the terrain
void VisualTerrain::updateTerrainMeshes(MeshBuilder& builder) {
    // Do nothing if nothing has changed.
    bool same_center = center_x == terrain->getCenterChunkX() &&
                       center_y == terrain->getCenterChunkY() &&
                       center_z == terrain->getCenterChunkZ();

    if (same_center)
        return;

    // Similar to terrain, we will store a mesh for every chunk, and on update,
    // shift our references so we only generate meshes for new chunks.
    const int old_center_x = center_x;
    const int old_center_y = center_y;
    const int old_center_z = center_z;

    center_x = terrain->getCenterChunkX();
    center_y = terrain->getCenterChunkY();
    center_z = terrain->getCenterChunkZ();

    // Iterate and find which chunk meshes can be reused
    std::memset(&chunk_meshes_helper[0], 0,
                chunk_meshes_helper.size() * sizeof(Mesh*));

    for (int i = 0; i < TERRAIN_CHUNK_COUNT; i++) {
        for (int j = 0; j < TERRAIN_CHUNK_COUNT; j++) {
            for (int k = 0; k < TERRAIN_CHUNK_COUNT; k++) {
                // Find our chunk index on x,z
                const int old_index_x = i + center_x - old_center_x;
                const int old_index_y = j + center_y - old_center_y;
                const int old_index_z = k + center_z - old_center_z;

                const bool in_x_bounds =
                    0 <= old_index_x && old_index_x < TERRAIN_CHUNK_COUNT;
                const bool in_y_bounds =
                    0 <= old_index_y && old_index_y < TERRAIN_CHUNK_COUNT;
                const bool in_z_bounds =
                    0 <= old_index_z && old_index_z < TERRAIN_CHUNK_COUNT;

                if (in_x_bounds && in_y_bounds && in_z_bounds) {
                    chunk_meshes_helper[index3DVector(i, j, k)] =
                        chunk_meshes[index3DVector(old_index_x, old_index_y,
                                                   old_index_z)];
                    chunk_meshes[index3DVector(old_index_x, old_index_y,
                                               old_index_z)] = nullptr;
                }
            }
        }
    }

    // Now, iterate through and
    // 1) Create new meshes that need to be created
    // 2) Destroy old meshes too far from our center
    const std::vector<Triangle>& triangle_pool = terrain->getTrianglePool();

    std::unordered_map<Vector3, UINT> vertex_map;

    for (int i = 0; i < TERRAIN_CHUNK_COUNT; i++) {
        for (int j = 0; j < TERRAIN_CHUNK_COUNT; j++) {
            for (int k = 0; k < TERRAIN_CHUNK_COUNT; k++) {
                const int index = index3DVector(i, j, k);

                // Free memory for old chunks
                if (chunk_meshes[index] != nullptr)
                    delete chunk_meshes[index];

                // Create new chunks
                if (chunk_meshes_helper[index] == nullptr) {
                    vertex_map.clear();
                    builder.reset();

                    const Chunk* chunk = terrain->getChunk(i, j, k);
                    if (chunk->dirty)
                        continue;

                    for (int tri = 0; tri < chunk->triangle_count; tri++) {
                        const Triangle& triangle =
                            triangle_pool[tri + chunk->triangle_start];

                        const Vector3 v0 = triangle.vertex(0);
                        const Vector3 v1 = triangle.vertex(1);
                        const Vector3 v2 = triangle.vertex(2);

                        // When generating our triangles, we will try to reuse
                        // vertices that have already been added, so that we can
                        // get smooth normals.
                        UINT i0;
                        if (vertex_map.contains(v0))
                            i0 = vertex_map[v0];
                        else {
                            i0 = builder.addVertex(v0);
                            vertex_map[v0] = i0;
                        }

                        UINT i1;
                        if (vertex_map.contains(v1))
                            i1 = vertex_map[v1];
                        else {
                            i1 = builder.addVertex(v1);
                            vertex_map[v1] = i1;
                        }

                        UINT i2;
                        if (vertex_map.contains(v2))
                            i2 = vertex_map[v2];
                        else {
                            i2 = builder.addVertex(v2);
                            vertex_map[v2] = i2;
                        }

                        builder.addTriangle(i0, i1, i2);
                    }

                    builder.regenerateNormals();

                    chunk_meshes_helper[index] = builder.generate();
                }
            }
        }
    }

    // Copy our helper array to the actual terrain array
    std::copy(chunk_meshes_helper.begin(), chunk_meshes_helper.end(),
              chunk_meshes.begin());
}

// GetTerrainMesh:
// Returns the terrain's mesh
const std::vector<Mesh*>& VisualTerrain::getTerrainMeshes() {
    return chunk_meshes;
}

int VisualTerrain::index3DVector(int x, int j, int k) {
    return k + TERRAIN_CHUNK_COUNT * (j + TERRAIN_CHUNK_COUNT * x);
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