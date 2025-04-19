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
VisualTerrainCallback::VisualTerrainCallback() = default;

void VisualTerrainCallback::initialize(ID3D11Device* _device) {
    device = _device;
}

// ExtractMesh:
// Pulls the mesh from the callback function, and sets its reference to nullptr.
Mesh* VisualTerrainCallback::extractMesh() {
    std::unique_lock<std::mutex> lock(mutex);
    Mesh* mesh = output_mesh;
    output_mesh = nullptr;

    dirty = false;

    return mesh;
}

// IsDirty:
// Returns if the callback has been updated or not.
bool VisualTerrainCallback::isDirty() {
    std::unique_lock<std::mutex> lock(mutex);
    return dirty;
}

// ReloadTerrainData:
// Callback function. When a terrain chunk is reloaded, reloads the chunk's mesh
// and stores it in output_mesh for the visual terrain interface to use.
void VisualTerrainCallback::reloadTerrainData(const TerrainChunk* chunk_data) {
    MeshBuilder builder = MeshBuilder();
    builder.reset();

    // Hashes vertices to their index in the mesh builder, so that we can reuse
    // vertices to get smooth normals.
    std::unordered_map<Vector3, UINT> vertex_map;
    vertex_map.clear();

    // Iterate through my chunk's triangles, and add them to the builder.
    UINT i0, i1, i2;
    for (const Triangle& triangle : chunk_data->triangles) {
        const Vector3 v0 = triangle.vertex(0);
        if (vertex_map.contains(v0))
            i0 = vertex_map[v0];
        else {
            i0 = builder.addVertex(v0);
            vertex_map[v0] = i0;
        }

        const Vector3 v1 = triangle.vertex(1);
        if (vertex_map.contains(v1))
            i1 = vertex_map[v1];
        else {
            i1 = builder.addVertex(v1);
            vertex_map[v1] = i1;
        }

        const Vector3 v2 = triangle.vertex(2);
        if (vertex_map.contains(v2))
            i2 = vertex_map[v2];
        else {
            i2 = builder.addVertex(v2);
            vertex_map[v2] = i2;
        }

        builder.addTriangle(i0, i1, i2);
    }

    // Add my chunk's border triangles, and add them to the builder.
    // These triangles are present only to make the normals consistent along the
    // borders.
    for (const Triangle& triangle : chunk_data->border_triangles) {
        const Vector3 v0 = triangle.vertex(0);
        if (vertex_map.contains(v0))
            i0 = vertex_map[v0];
        else {
            i0 = builder.addVertex(v0);
            vertex_map[v0] = i0;
        }

        const Vector3 v1 = triangle.vertex(1);
        if (vertex_map.contains(v1))
            i1 = vertex_map[v1];
        else {
            i1 = builder.addVertex(v1);
            vertex_map[v1] = i1;
        }

        const Vector3 v2 = triangle.vertex(2);
        if (vertex_map.contains(v2))
            i2 = vertex_map[v2];
        else {
            i2 = builder.addVertex(v2);
            vertex_map[v2] = i2;
        }

        builder.addTriangle(i0, i1, i2);
    }

    // Generate our normals, and remove the border triangles so that we don't
    // have z-fighting
    builder.regenerateNormals();
    builder.popTriangles(chunk_data->border_triangles.size());

    // Create our mesh.
    // If there is already a mesh, destroy it and replace it.
    {
        std::unique_lock<std::mutex> lock(mutex);

        dirty = true;

        if (output_mesh != nullptr)
            delete output_mesh;
        output_mesh = builder.generate(device);
    }
}

VisualTerrain::VisualTerrain(Terrain* _terrain, ID3D11Device* device)
    : terrain(_terrain), callbacks() {
    // Set all current chunk_meshes to null, and initialize my terrain
    // callbacks.
    for (int i = 0; i < TERRAIN_CHUNK_COUNT; i++) {
        for (int j = 0; j < TERRAIN_CHUNK_COUNT; j++) {
            for (int k = 0; k < TERRAIN_CHUNK_COUNT; k++) {
                chunk_meshes[i][j][k] = nullptr;

                callbacks[i][j][k].initialize(device);
                terrain->registerTerrainCallback(i, j, k, &callbacks[i][j][k]);
            }
        }
    }
}
VisualTerrain::~VisualTerrain() {
    for (int i = 0; i < TERRAIN_CHUNK_COUNT; i++) {
        for (int j = 0; j < TERRAIN_CHUNK_COUNT; j++) {
            for (int k = 0; k < TERRAIN_CHUNK_COUNT; k++) {
                delete chunk_meshes[i][j][k];
            }
        }
    }
}

// GenerateTerrainMesh:
// Generates the mesh for the terrain
void VisualTerrain::updateTerrainMeshes() {
    // Iterate through my callbacks. If they have a mesh, overwrite what we
    // currently have.
    for (int i = 0; i < TERRAIN_CHUNK_COUNT; i++) {
        for (int j = 0; j < TERRAIN_CHUNK_COUNT; j++) {
            for (int k = 0; k < TERRAIN_CHUNK_COUNT; k++) {
                VisualTerrainCallback& callback = callbacks[i][j][k];

                if (callback.isDirty()) {
                    if (chunk_meshes[i][j][k] != nullptr)
                        delete chunk_meshes[i][j][k];
                    chunk_meshes[i][j][k] = callback.extractMesh();
                }
            }
        }
    }

    // Now, iterate through all of my output meshes and add them to the output
    // vector.
    output_meshes.clear();
    for (int i = 0; i < TERRAIN_CHUNK_COUNT; i++) {
        for (int j = 0; j < TERRAIN_CHUNK_COUNT; j++) {
            for (int k = 0; k < TERRAIN_CHUNK_COUNT; k++) {
                if (chunk_meshes[i][j][k] != nullptr)
                    output_meshes.push_back(chunk_meshes[i][j][k]);
            }
        }
    }
}

// GetTerrainMesh:
// Returns the terrain's mesh
const std::vector<Mesh*>& VisualTerrain::getTerrainMeshes() {
    return output_meshes;
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