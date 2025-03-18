#include "VisualTerrain.h"

#include "datamodel/TreeGenerator.h"
#include "math/Compute.h"

constexpr int SAMPLE_COUNT = 20;

namespace Engine {
using namespace Datamodel;

namespace Graphics {
VisualTerrain::VisualTerrain(TerrainChunk* _terrain, MeshBuilder* mesh_builder)
    : terrain(_terrain) {
    terrain_mesh = generateTerrainMesh(*mesh_builder);

    for (const Vector2& tree_loc : terrain->getTreeLocations()) {
        tree_meshes.push_back(generateTreeMesh(*mesh_builder, tree_loc));
    }

    markedToDestroy = false;
}

VisualTerrain::~VisualTerrain() {
    delete terrain_mesh;

    for (Mesh* tree : tree_meshes)
        delete tree;
}

void VisualTerrain::destroy() { markedToDestroy = true; }

bool VisualTerrain::markedForDestruction() const { return markedToDestroy; }

// GenerateTerrainMesh:
// Generates the mesh for the terrain
Mesh* VisualTerrain::generateTerrainMesh(MeshBuilder& builder) {
    builder.reset();

    for (int i = 0; i < SAMPLE_COUNT; i++) {
        for (int j = 0; j < SAMPLE_COUNT; j++) {
            const float x =
                HEIGHT_MAP_XZ_SIZE * i / (SAMPLE_COUNT - 1) + terrain->getX();
            const float z =
                HEIGHT_MAP_XZ_SIZE * j / (SAMPLE_COUNT - 1) + terrain->getZ();

            const float y = terrain->sampleTerrainHeight(x, z);

            builder.addVertex(Vector3(x, y, z));
        }
    }

    for (int i = 0; i < SAMPLE_COUNT; i++) {
        for (int j = 0; j < SAMPLE_COUNT; j++) {
            const int v0 = i * SAMPLE_COUNT + j;
            const int v1 = (i + 1) * SAMPLE_COUNT + j;
            const int v2 = i * SAMPLE_COUNT + (j + 1);
            const int v3 = (i - 1) * SAMPLE_COUNT + j;
            const int v4 = i * SAMPLE_COUNT + (j - 1);

            // NE triangle
            if (i < SAMPLE_COUNT - 1 && j < SAMPLE_COUNT - 1)
                builder.addTriangle(v0, v2, v1);

            // NW triangle
            if (i > 0 && j < SAMPLE_COUNT - 1)
                builder.addTriangle(v0, v3, v2);

            // SW triangle
            if (i > 0 && j > 0)
                builder.addTriangle(v0, v4, v3);

            // SE triangle
            if (i < SAMPLE_COUNT - 1 && j > 0)
                builder.addTriangle(v0, v1, v4);
        }
    }

    builder.regenerateNormals();
    return builder.generate();
}

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

} // namespace Graphics
} // namespace Engine