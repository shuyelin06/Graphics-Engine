#pragma once

#include <unordered_set>
#include <vector>

#include "math/Vector3.h"

namespace Engine {
using namespace Math;

namespace Graphics {
// Octree:
// Implementation of an octree, which divides 3D space into recursively
// subdivided cubes.
// To use, create the Octree. When you want to update it, pass in an
// OctreeUpdater. This should return a requested LOD for a node, and the tree
// will:
// - If Node.isLeaf() and Requested LOD < Node Depth, we want more detail
//   so we divide the node and repeat.
// - If !Node.isLeaf() and Requested LOD > Node Depth, we want less detail so we
//   merge the node.
// TODO: Add a node allocator so everything is contiguous.
class Octree;
typedef unsigned int OctreeNodeID;

struct OctreeNode {
    Octree* octree;

    // Unique ID. Every node has a unique ID we can reference it by
    OctreeNodeID uniqueID;

    // Node Description in Space
    Vector3 center;
    float extents;

    unsigned int depth; // Depth = 0 is the smallest node possible
    OctreeNode* children[8];

    unsigned int getDepth() const;
    bool isLeaf() const;

    // Create 8 children with depth - 1.
    // If depth is 0 we cannot divide any more.
    void divide();

    // Remove children, effectively "merging" the nodes into
    // this one.
    void merge();

    void initialize(const Vector3& center, float extents, unsigned int depth);

  private:
    // All allocations / deallocations go through Octree
    friend class Octree;
    OctreeNode(Octree* octree, unsigned int uniqueID);
    ~OctreeNode();
};

class OctreeUpdater {
    // The LOD rings are centered around this point.s
    Vector3 point_of_focus;
    // Array of ascending distances, where index i
    // is the radius in which we want LOD i or smaller.
    std::vector<float> lod_rings;

    friend class Octree;
    OctreeUpdater(unsigned int maxDepth);

  public:
    ~OctreeUpdater();

    void updatePointOfFocus(const Vector3& point_of_focus);
    void updateLODDistance(unsigned int lod, float radius);

    // As nodes are boxes in 3D space, a node can intersect multiple LOD rings.
    // We want to return the minimum LOD needed for the node for the Octree to
    // work properly.
    unsigned int smallestLODInNode(const OctreeNode& node) const;
};

// For external classes, octree tracks 2 things:
// 1) HashSet of active leaves in the octree
// 2) List of newly created leaves in the octree, whether just allocated or
// divided Terrain can use the list to kick off new async chunk building
// jobs, and use the hashset to determine what chunk meshes are no longer
// valid (to free memory).
class Octree {
  private:
    // Map of Node ID --> Node
    std::unordered_map<OctreeNodeID, OctreeNode*> node_map;

    unsigned int idCounter;

    OctreeNode* root;

    // Config
    struct {
        const unsigned int maxDepth; // # times we can divide
        const float voxelSize;       // Size of the smallest node
    } config;

  public:
    Octree(unsigned int maxDepth, float voxelSize);
    ~Octree();

    void update(const OctreeUpdater& lodRequestor);
    void debugDrawLeaves();

    OctreeUpdater getUpdater();

    const std::vector<OctreeNodeID>& getActiveLeaves() const;
    bool isNodeLeaf(OctreeNodeID id) const;

    const std::unordered_map<OctreeNodeID, OctreeNode*>& getNodeMap() const;
    const OctreeNode* getNode(OctreeNodeID id) const;

  private:
    void updateHelper(OctreeNode* node, const OctreeUpdater& lodRequestor);
    void debugDrawLeavesHelper(OctreeNode* node);

    friend struct OctreeNode;
    OctreeNode* allocateNode();
    void destroyNode(OctreeNode* node);

    void trackNodeAsLeaf(const OctreeNode* node);
    void removeNodeAsLeaf(const OctreeNode* node);
};

} // namespace Graphics
} // namespace Engine