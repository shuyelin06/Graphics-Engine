#include "Octree.h"

#include "../VisualDebug.h"

#include <assert.h>

namespace Engine {
namespace Graphics {
OctreeNode::OctreeNode(const Vector3& _center, float _extents,
                       unsigned int _depth)
    : children{nullptr} {
    center = _center;
    extents = _extents;
    depth = _depth;
}
OctreeNode::~OctreeNode() {
    if (!isLeaf()) {
        for (int i = 0; i < 8; i++) {
            assert(children[i] != nullptr);
            delete children[i];
        }
    }
}

unsigned int OctreeNode::getDepth() const { return depth; }
bool OctreeNode::isLeaf() const { return children[0] == nullptr; }

void OctreeNode::divide() {
    assert(isLeaf());
    if (isLeaf() && depth > 0) {
        // clang-format off
        const static Vector3 child_centers[8] = {
            Vector3(1, 1, 1),   Vector3(-1, 1, 1),
            Vector3(1, -1, 1),  Vector3(-1, -1, 1),
            Vector3(1, 1, -1),  Vector3(-1, 1, -1),
            Vector3(1, -1, -1), Vector3(-1, -1, -1),
        };
        // clang-format on
        const float child_extents = extents / 2;
        const unsigned int child_depth = depth - 1;

        for (int i = 0; i < 8; i++) {
            children[i] =
                new OctreeNode(center + child_centers[i] * child_extents,
                               child_extents, child_depth);
        }
    }
}

void OctreeNode::merge() {
    assert(!isLeaf());
    if (!isLeaf()) {
        for (int i = 0; i < 8; i++) {
            delete children[i];
            children[i] = nullptr;
        }
    }
}

OctreeUpdater::OctreeUpdater() : lod_rings{} {
    point_of_focus = Vector3(0, 0, 0);
}
OctreeUpdater::~OctreeUpdater() = default;

void OctreeUpdater::updatePointOfFocus(const Vector3& _point_of_focus) {
    point_of_focus = _point_of_focus;
}
void OctreeUpdater::updateLODDistance(unsigned int lod, float radius) {
    lod_rings[lod] = radius;
}

unsigned int OctreeUpdater::smallestLODInNode(const OctreeNode& node) const {
    auto isLODInNode = [this](const OctreeNode& node, float lod_radius) {
        // 1) Find the closest point on the box to the sphere
        const Vector3 box_min =
            node.center - Vector3(node.extents, node.extents, node.extents);
        const Vector3 box_max =
            node.center + Vector3(node.extents, node.extents, node.extents);

        Vector3 closest_point;
        for (int dimension = 0; dimension < 3; dimension++) {
            if (point_of_focus[dimension] < box_min[dimension]) {
                closest_point[dimension] = box_min[dimension];
            } else if (box_min[dimension] <= point_of_focus[dimension] &&
                       point_of_focus[dimension] <= box_max[dimension]) {
                closest_point[dimension] = point_of_focus[dimension];
            } else if (box_max[dimension] < point_of_focus[dimension]) {
                closest_point[dimension] = box_max[dimension];
            } else
                assert(false); // One of the above should've happened
        }

        // 2) Now, see if this point is contained within the sphere
        return (closest_point - point_of_focus).magnitude() <= lod_radius;
    };

    for (int i = 0; i < OCTREE_MAX_DEPTH - 1; i++) {
        if (isLODInNode(node, lod_rings[i])) {
            return i;
        }
    }

    return OCTREE_MAX_DEPTH;
}

Octree::Octree() {
    const float root_extents = OCTREE_VOXEL_SIZE * (1 << OCTREE_MAX_DEPTH);
    root = new OctreeNode(Vector3(0, 0, 0), root_extents, OCTREE_MAX_DEPTH);
}
Octree::~Octree() { delete root; }

void Octree::update(const OctreeUpdater& lodRequestor) {
    updateHelper(root, lodRequestor);
}
void Octree::updateHelper(OctreeNode* node, const OctreeUpdater& lodRequestor) {
    // Nodes are boxes in 3D space, so a node can intersect multiple LOD
    // spheres. First get the smallest (highest detail) LOD sphere that the node
    // intersects.
    const unsigned int smallestRequestedLOD =
        lodRequestor.smallestLODInNode(*node);

    // If we have a leaf, and the requested LOD is < the node depth, we
    // divide the node and repeat.
    if (smallestRequestedLOD < node->getDepth()) {
        if (node->isLeaf()) {
            node->divide();
        }
        assert(!node->isLeaf());
        for (int i = 0; i < 8; i++) {
            updateHelper(node->children[i], lodRequestor);
        }
    } else if (node->getDepth() <= smallestRequestedLOD) {
        if (!node->isLeaf()) {
            // If we do not have a leaf, and the requested LOD is > the node
            // depth, we can merge the node
            node->merge();
        }
    }
}

void Octree::debugDrawLeaves() { debugDrawLeavesHelper(root); }
void Octree::debugDrawLeavesHelper(OctreeNode* node) {
    if (node->isLeaf()) {
        const float& extents = node->extents;
        const Vector3 box_min =
            node->center - Vector3(extents, extents, extents);
        const Vector3 box_max =
            node->center + Vector3(extents, extents, extents);

        VisualDebug::DrawBox(box_min, box_max);
    } else {
        for (int i = 0; i < 8; i++) {
            debugDrawLeavesHelper(node->children[i]);
        }
    }
}

} // namespace Graphics
} // namespace Engine