#include "Octree.h"

#include "../VisualDebug.h"

#include <assert.h>

namespace Engine {
namespace Graphics {
OctreeNode::OctreeNode() : children{nullptr} {
    octree = nullptr;
    uniqueID = INVALID_NODE_ID;
}
OctreeNode::~OctreeNode() = default;

void OctreeNode::initialize(const Vector3& _center, float _extents,
                            unsigned int _depth) {
    center = _center;
    extents = _extents;
    depth = _depth;
}

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
            children[i] = octree->allocateNode();
            children[i]->initialize(center + child_centers[i] * child_extents,
                                    child_extents, child_depth);
        }

        octree->removeNodeAsLeaf(this);
        octree->trackDivisionOperation(this);
    }
}

void OctreeNode::merge() {
    // Do before so that we can get the children IDs
    octree->trackMergeOperation(this);
    if (!isLeaf()) {
        destroyAllChildren();
        octree->trackNodeAsLeaf(this);
    }
}
void OctreeNode::destroyAllChildren() {
    if (!isLeaf()) {
        for (int i = 0; i < 8; i++) {
            children[i]->destroyAllChildren();
            octree->destroyNode(children[i]);
            children[i] = nullptr;
        }
    }
}

OctreeUpdater::OctreeUpdater(unsigned int maxDepth) : lod_rings(maxDepth) {
    point_of_focus = Vector3(0, 0, 0);
}
OctreeUpdater::~OctreeUpdater() = default;

void OctreeUpdater::updatePointOfFocus(const Vector3& _point_of_focus) {
    point_of_focus = _point_of_focus;
}
void OctreeUpdater::updateLODDistance(unsigned int lod, float radius) {
    assert(lod < lod_rings.size());
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

    for (int i = 0; i < lod_rings.size(); i++) {
        if (isLODInNode(node, lod_rings[i])) {
            return i;
        }
    }

    return lod_rings.size();
}

Octree::Octree(unsigned int _maxDepth, float _voxelSize)
    : config{_maxDepth, _voxelSize} {
    idCounter = INVALID_NODE_ID + 1;

    const float root_extents = config.voxelSize * (1 << config.maxDepth);
    root = allocateNode();
    root->initialize(Vector3(0, 0, 0), root_extents, config.maxDepth);
}
Octree::~Octree() { destroyNode(root); }

OctreeNode* Octree::allocateNode() {
    const unsigned int nodeID = idCounter++;
    OctreeNode* newNode = allocator.allocate();
    newNode->octree = this;
    newNode->uniqueID = nodeID;

    assert(!node_map.contains(nodeID));
    node_map[nodeID] = newNode;

    trackNodeAsLeaf(newNode);

    return newNode;
}

void Octree::destroyNode(OctreeNode* node) {
    assert(node_map.contains(node->uniqueID));
    node_map.erase(node->uniqueID);

    if (node->isLeaf()) {
        removeNodeAsLeaf(node);
    }

    allocator.free(node);
}

void Octree::trackNodeAsLeaf(const OctreeNode* node) {
    // Does nothing (add code here if needed)
}

void Octree::removeNodeAsLeaf(const OctreeNode* node) {
    // Does nothing (add code here if needed)
}

void Octree::trackDivisionOperation(const OctreeNode* node) {
    operations.resize(operations.size() + 1);
    auto& operation = operations.back();

    operation.type = OctreeNode::Operation::DIVIDE;
    operation.parent = node->uniqueID;
    for (int i = 0; i < 8; i++) {
        operation.children[i] = node->children[i]->uniqueID;
    }
}
void Octree::trackMergeOperation(const OctreeNode* node) {
    operations.resize(operations.size() + 1);
    auto& operation = operations.back();

    operation.type = OctreeNode::Operation::MERGE;
    operation.parent = node->uniqueID;
    for (int i = 0; i < 8; i++) {
        operation.children[i] = node->children[i]->uniqueID;
    }
}

void Octree::resetOctree(unsigned int _maxDepth, float _voxelSize) {
    config = {_maxDepth, _voxelSize};

    delete root;

    const float root_extents = config.voxelSize * (1 << config.maxDepth);
    root = allocateNode();
    root->initialize(Vector3(0, 0, 0), root_extents, config.maxDepth);
}
void Octree::update(const OctreeUpdater& lodRequestor) {
    operations.clear();
    updateHelper(root, lodRequestor);
}
void Octree::updateHelper(OctreeNode* node, const OctreeUpdater& lodRequestor) {
    // Nodes are boxes in 3D space, so a node can intersect multiple LOD
    // spheres. First get the smallest (highest detail) LOD sphere that the
    // node intersects.
    const unsigned int smallestRequestedLOD =
        lodRequestor.smallestLODInNode(*node);

    // If we have a leaf, and the requested LOD is < the node depth, we
    // divide the node and repeat.
    if (smallestRequestedLOD < node->depth) {
        if (node->isLeaf()) {
            node->divide();
        }
        assert(!node->isLeaf());
        for (int i = 0; i < 8; i++) {
            updateHelper(node->children[i], lodRequestor);
        }
    } else if (node->depth <= smallestRequestedLOD) {
        if (!node->isLeaf()) {
            // If we do not have a leaf, and the requested LOD is > the node
            // depth, we can merge the node
            node->merge();
        }
    }
}

OctreeUpdater Octree::getUpdater() {
    return std::move(OctreeUpdater(config.maxDepth));
}

const std::vector<OctreeNode::Operation>& Octree::getOperations() const {
    return operations;
}
const std::unordered_map<OctreeNodeID, OctreeNode*>&
Octree::getNodeMap() const {
    return node_map;
}

const OctreeNode* Octree::getNode(OctreeNodeID id) const {
    if (node_map.contains(id))
        return node_map.at(id);
    else
        return nullptr;
}

bool Octree::isNodePresent(OctreeNodeID id) const {
    return node_map.contains(id);
}

bool Octree::isNodeLeaf(const OctreeNodeID id) const {
    if (isNodePresent(id)) {
        return node_map.at(id)->isLeaf();
    } else {
        return false;
    }
}

} // namespace Graphics
} // namespace Engine