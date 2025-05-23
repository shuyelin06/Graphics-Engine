#include "TreeGenerator.h"

#include "math/Compute.h"
#include "math/Quaternion.h"

/*

int generateTreeMeshHelper(MeshBuilder& builder,
                           const std::vector<TreeStructure>& grammar, int index,
                           const Vector3& position, const Vector2& rotation) {
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

void generateTreeMesh(MeshBuilder& builder,
                      const std::vector<TreeStructure>& grammar,
                      const Vector3& offset) {
    // Rotation stores (phi, theta), spherical angles. rho is assumed to be 1.
    generateTreeMeshHelper(builder, grammar, 0, offset, Vector2(0, 0));
}
*/

namespace Engine {
using namespace Math;

namespace Datamodel {
TreeGenerator::TreeGenerator() = default;
TreeGenerator::~TreeGenerator() = default;

const std::vector<TreeStructure>& TreeGenerator::getTree() { return grammar; }

// GenerateTree:
// Randomly generate a tree using the tree generator.
void TreeGenerator::generateTree() {
    grammar.clear();

    branch_depth = 1;

    addTrunk();
    generateTreeHelper();
}

void TreeGenerator::generateTreeHelper() {
    const float prob_trunk = trunkProbability();
    const float prob_branch = branchProbability();
    const float prob_leaf = leafProbability();

    // Do a random experiment to see where we go
    const float total = prob_trunk + prob_branch + prob_leaf;
    const float random = Random(0.0f, total);

    // T -> tT: The trunk grows
    if (0 <= random && random <= prob_trunk) {
        addTrunk();
        generateTreeHelper();
    }
    // T -> tbTT: The trunk branches
    else if (prob_trunk < random && random <= prob_trunk + prob_branch) {
        addTrunk();

        const int num_branches = Random(1, 3);
        for (int i = 0; i < num_branches; i++) {
            addBranch();
            addTrunk();
            generateTreeHelper();
        }

        generateTreeHelper();
    }
    // T -> l: The trunk ends
    else {
        addLeaf();
    }
}

float TreeGenerator::trunkProbability() const { return 0.5f; }

float TreeGenerator::branchProbability() const { return 0.15f; }

float TreeGenerator::leafProbability() const { return 0.35f; }

void TreeGenerator::addTrunk() {
    if (grammar.size() > 0 && grammar[grammar.size() - 1].token == TRUNK) {
        TreeStructure& tree = grammar[grammar.size() - 1];
        tree.trunk_data.trunk_thickess += 5.f;
        tree.trunk_data.trunk_length += 5.f / branch_depth;
    } else {
        TreeStructure tree = {};
        tree.token = TRUNK;
        tree.trunk_data.trunk_thickess = 3.f;
        tree.trunk_data.trunk_length = 5.f / branch_depth;

        grammar.push_back(tree);
    }
}
void TreeGenerator::addLeaf() {
    TreeStructure tree = {};
    tree.token = LEAF;
    tree.leaf_data.leaf_density = Random(5.0f, 10.f);

    grammar.push_back(tree);

    branch_depth--;
}
void TreeGenerator::addBranch() {
    TreeStructure tree = {};
    tree.token = BRANCH;
    // Branching angle can only be in the northern hemisphere
    // (trees grow up, after all)
    tree.branch_data.branch_angle_phi = Random(0.f, PI / 4.f);
    tree.branch_data.branch_angle_theta = Random(0.f, 2 * PI);

    grammar.push_back(tree);
    branch_depth++;
}

// DebugDrawTree:
// Uses visual debug to draw the tree. The helper function returns the index
// it left off at for previous calls to continue the iteration with.
#if defined(_DEBUG)
using namespace Engine::Graphics;

void TreeGenerator::debugDrawTree(const Vector3& offset) {
    // Rotation stores (phi, theta), spherical angles. rho is assumed to be 1.
    debugDrawTreeHelper(0, offset, Vector2(0, 0));
}

int TreeGenerator::debugDrawTreeHelper(int index, const Vector3& position,
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

        VisualDebug::DrawLine(position, next_pos,
                              Color(150.f / 255.f, 75.f / 255.f, 0));
        return debugDrawTreeHelper(index + 1, next_pos, rotation);
    } break;

    case BRANCH: {
        const Vector2 new_rotation =
            rotation + Vector2(tree.branch_data.branch_angle_phi,
                               tree.branch_data.branch_angle_theta);

        const int next_index =
            debugDrawTreeHelper(index + 1, position, new_rotation);
        return debugDrawTreeHelper(next_index, position, rotation);
    } break;

    case LEAF: {
        VisualDebug::DrawPoint(position, 2.0f, Color::Green());
        return index + 1;
    } break;
    }

    return -1;
}
#endif
} // namespace Datamodel
} // namespace Engine