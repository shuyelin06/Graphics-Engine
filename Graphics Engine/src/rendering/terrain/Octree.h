#pragma once

#include <unordered_set>
#include <vector>

#include "core/PoolAllocator.h"
#include "math/Vector3.h"

#include "rendering/core/Mesh.h"

#include "ChunkBuilderJob.h"

namespace Engine {
using namespace Math;

namespace Graphics {
// Octree:
// TODO Rename to TerrainMeshLoader
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
class TerrainMeshLoader;
typedef unsigned int TerrainNodeID;

static constexpr TerrainNodeID INVALID_NODE_ID = 0;

struct TerrainNode {
    TerrainMeshLoader* octree;

    // Unique ID. Every node has a unique ID we can reference it by
    TerrainNodeID uniqueID;

    // Node Description in Space
    Vector3 center;
    float extents;
    unsigned int depth; // Depth = 0 is the smallest node possible

    // Node Renderable Mesh
    std::shared_ptr<Mesh> mesh = nullptr;
    bool loaded = false;

    // Node Children
    TerrainNode* children[8];
    bool isLeaf() const;

    // Node Operations
    // - Divide: Create 8 children with depth - 1. If depth is 0 we cannot
    // divide any more.
    // - Merge: Remove children, making this node the leaf. Effective "merges"
    // the nodes into this one.
    void divide();
    void merge();

    TerrainNode();
    ~TerrainNode();

    void initialize(const Vector3& center, float extents, unsigned int depth);

  private:
    void destroyAllChildren();
};

class OctreeUpdater {
    // The LOD rings are centered around this point.s
    Vector3 point_of_focus;
    // Array of ascending distances, where index i
    // is the radius in which we want LOD i or smaller.
    std::vector<float> lod_rings;

    friend class TerrainMeshLoader;
    OctreeUpdater(unsigned int maxDepth);

  public:
    ~OctreeUpdater();

    void updatePointOfFocus(const Vector3& point_of_focus);
    void updateLODDistance(unsigned int lod, float radius);

    // As nodes are boxes in 3D space, a node can intersect multiple LOD rings.
    // We want to return the minimum LOD needed for the node for the Octree to
    // work properly.
    unsigned int smallestLODInNode(const TerrainNode& node) const;
};

// For external classes, octree tracks 2 things:
// 1) HashSet of active leaves in the octree
// 2) List of newly created leaves in the octree, whether just allocated or
// divided Terrain can use the list to kick off new async chunk building
// jobs, and use the hashset to determine what chunk meshes are no longer
// valid (to free memory).
class TerrainMeshLoader {
  private:
    // Root of Octree
    TerrainNode* root;
    // Map of Node ID --> Node for Easy Access into Octree
    std::unordered_map<TerrainNodeID, TerrainNode*> node_map;

    // Mesh Loading
    std::vector<TerrainNodeID> dirty_nodes;

    std::vector<std::unique_ptr<ChunkBuilderJob>> jobs;
    std::vector<int> inactive_jobs;

    // Allocations
    TerrainNodeID idCounter;
    PoolAllocator<TerrainNode, 1000> allocator;

    // Config
    struct {
        unsigned int maxDepth; // # times we can divide
        float voxelSize;       // Size of the smallest node
    } config;

  public:
    TerrainMeshLoader(unsigned int maxDepth, float voxelSize);
    ~TerrainMeshLoader();

    void resetOctree(unsigned int _maxDepth, float _voxelSize);

    void updateOctree(const OctreeUpdater& lodRequestor);
    void serveBuildRequests(TerrainGenerator* generator,
                            ResourceManager* resource_managers);
    void findValidMeshes(std::vector<Mesh*>& meshes);

    OctreeUpdater getUpdater();

    const std::unordered_map<TerrainNodeID, TerrainNode*>& getNodeMap() const;

    const TerrainNode* getNode(TerrainNodeID id) const;
    bool isNodePresent(TerrainNodeID id) const;
    bool isNodeLeaf(TerrainNodeID id) const;

  private:
    void findValidMeshesHelper(TerrainNode* node, std::vector<Mesh*>& meshes);
    void updateOctreeHelper(TerrainNode* node,
                            const OctreeUpdater& lodRequestor);

    friend struct TerrainNode;
    TerrainNode* allocateNode();
    void destroyNode(TerrainNode* node);
};

} // namespace Graphics
} // namespace Engine