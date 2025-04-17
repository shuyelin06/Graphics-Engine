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
    // These size are NOT to be changed. They're coded to match the size of
    // the 3D array in Terrain, but given as a vector so the VisualSystem has an
    // easier time reading the data.
    chunk_meshes.resize(TERRAIN_CHUNK_COUNT * TERRAIN_CHUNK_COUNT *
                        TERRAIN_CHUNK_COUNT);

    for (int i = 0; i < TERRAIN_CHUNK_COUNT; i++) {
        for (int j = 0; j < TERRAIN_CHUNK_COUNT; j++) {
            for (int k = 0; k < TERRAIN_CHUNK_COUNT; k++) {
                dirty[i][j][k] = true;
            }
        }
    }
}
VisualTerrain::~VisualTerrain() {
    for (int i = 0; i < chunk_meshes.size(); i++)
        if (chunk_meshes[i] != nullptr)
            delete chunk_meshes[i];
}

// GenerateTerrainMesh:
// Generates the mesh for the terrain
void VisualTerrain::updateTerrainMeshes(MeshBuilder& builder) {
    // Similar to terrain, we will store a mesh for every chunk, and update
    // them if they are dirty.
    std::unordered_map<Vector3, UINT> vertex_map;

    for (int i = 0; i < TERRAIN_CHUNK_COUNT; i++) {
        for (int j = 0; j < TERRAIN_CHUNK_COUNT; j++) {
            for (int k = 0; k < TERRAIN_CHUNK_COUNT; k++) {
                const Chunk* chunk = terrain->getChunk(i, j, k);
                const int index = index3DVector(i, j, k);

                // If the chunk is dirty, remove its mesh
                if (chunk->isDirty()) {
                    if (chunk_meshes[index] != nullptr) {
                        delete chunk_meshes[index];
                        chunk_meshes[index] = nullptr;

                        dirty[i][j][k] = true;
                    }
                }
                // Otherwise, if the chunk is not dirty and the mesh is not
                // created, create it.
                else {
                    if (dirty[i][j][k]) {
                        vertex_map.clear();
                        builder.reset();

                        for (const Triangle& triangle : chunk->triangles) {
                            const Vector3 v0 = triangle.vertex(0);
                            const Vector3 v1 = triangle.vertex(1);
                            const Vector3 v2 = triangle.vertex(2);

                            // When generating our triangles, we will try to
                            // reuse vertices that have already been added, so
                            // that we can get smooth normals.
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

                        chunk_meshes[index] = builder.generate();
                        dirty[i][j][k] = false;
                    }
                }
            }
        }
    }
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