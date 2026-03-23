#include "TerrainOctree.h"

#include <assert.h>
#include <unordered_set>
#include <vector>

#include "core/PoolAllocator.h"
#include "core/ThreadPool.h"
#include "math/Vector3.h"

#include "rendering/core/Mesh.h"
#include "rendering/resources/ResourceManager.h"

#include "../VisualDebug.h"
#include "ChunkBuilderJob.h"

constexpr int MAX_CHUNK_JOBS = 16;

namespace Engine {
using namespace Math;
namespace Graphics {
typedef unsigned int TerrainNodeID;
static constexpr TerrainNodeID kInvalidNodeID = 0;

struct TerrainNode {
    // Unique ID. Every node has a unique ID we can reference it by
    TerrainNodeID uniqueID = kInvalidNodeID;

    // Node Description in Space
    Vector3 center;
    float extents;
    unsigned int depth; // Depth = 0 is the smallest node possible

    // Node Renderable Mesh
    std::shared_ptr<Mesh> mesh = nullptr;
    bool loaded = false;

    // Node Children
    TerrainNode* children[8]{nullptr};
    bool isLeaf() const;
};

bool TerrainNode::isLeaf() const { return children[0] == nullptr; }

class LODSelector {
    // The LOD rings are centered around this point.s
    Vector3 point_of_focus;
    // Array of ascending distances, where index i
    // is the radius in which we want LOD i or smaller.
    std::vector<float> lod_rings;

  public:
    LODSelector();
    ~LODSelector();

    void configure(const Vector3& pointOfFocus, float voxelSize,
                   unsigned int maxDepth);

    // As nodes are boxes in 3D space, a node can intersect multiple LOD rings.
    // We want to return the minimum LOD needed for the node for the Octree to
    // work properly.
    unsigned int smallestLODInNode(const TerrainNode& node) const;
};

LODSelector::LODSelector() = default;
LODSelector::~LODSelector() = default;

void LODSelector::configure(const Vector3& pointOfFocus, float voxelSize,
                            unsigned int maxDepth) {
    point_of_focus = pointOfFocus;
    lod_rings.resize(maxDepth);

    float accumulated_size = voxelSize * 1.5f;
    for (int i = 0; i < maxDepth - 1; i++) {
        accumulated_size *= 2;
        lod_rings[i] = accumulated_size;
    }
}

unsigned int LODSelector::smallestLODInNode(const TerrainNode& node) const {
    auto isLODInNode = [this](const TerrainNode& node, float lod_radius) {
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

using SDFGeneratorDelegate = TerrainOctree::SDFGeneratorDelegate;
class TerrainOctreeImpl {
  private:
    SDFGeneratorDelegate* sdfGenerator;
    ResourceManager* resourceManager;

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
    constexpr static unsigned int kDefaultMaxDepth = 8;
    unsigned int maxDepth = kDefaultMaxDepth;

    constexpr static float kDefaultVoxelSize = 25.f;
    float voxelSize = kDefaultVoxelSize;

  public:
    TerrainOctreeImpl(SDFGeneratorDelegate* sdfGenerator,
                      ResourceManager* resourceManager);
    ~TerrainOctreeImpl();

    void resetOctree(unsigned int _maxDepth, float _voxelSize);

    void update(const Vector3& pointOfFocus);
    void pullTerrainMeshes(std::vector<Mesh*>& meshes);

  private:
    // Update
    void updateLODsRecursive(TerrainNode* node,
                             const LODSelector& lodRequestor);
    void serveBuildRequests();

    // Node Management
    friend struct TerrainNode;
    TerrainNode* allocateNode(const Vector3& center, const float extents,
                              unsigned int depth);
    void destroyNode(TerrainNode* node);

    // Node Operations
    // - Divide: Create 8 children with depth - 1. If depth is 0 we cannot
    // divide any more.
    // - Merge: Remove children, making this node the leaf. Effective "merges"
    // the nodes into this one.
    void divideNode(TerrainNode* node);
    void mergeNode(TerrainNode* node);

    void destroyAllChildren(TerrainNode* node);
};

std::unique_ptr<TerrainOctree>
TerrainOctree::create(SDFGeneratorDelegate* sdfGenerator,
                      ResourceManager* resourceManager) {
    std::unique_ptr<TerrainOctree> ptr =
        std::unique_ptr<TerrainOctree>(new TerrainOctree());
    ptr->mImpl =
        std::make_unique<TerrainOctreeImpl>(sdfGenerator, resourceManager);
    return std::move(ptr);
}

TerrainOctree::TerrainOctree() = default;
TerrainOctree::~TerrainOctree() = default;

void TerrainOctree::resetOctree(unsigned int _maxDepth, float _voxelSize) {
    mImpl->resetOctree(_maxDepth, _voxelSize);
}

void TerrainOctree::update(const Vector3& pointOfFocus) {
    mImpl->update(pointOfFocus);
}
void TerrainOctree::pullTerrainMeshes(std::vector<Mesh*>& meshes) {
    mImpl->pullTerrainMeshes(meshes);
}

TerrainOctreeImpl::TerrainOctreeImpl(SDFGeneratorDelegate* _sdfGenerator,
                                     ResourceManager* _resourceManager) {
    sdfGenerator = _sdfGenerator;
    resourceManager = _resourceManager;

    idCounter = kInvalidNodeID + 1;

    // Create MAX_CHUNK_JOBS schunk update jobs.
    for (int i = 0; i < MAX_CHUNK_JOBS; i++) {
        jobs.push_back(std::make_unique<ChunkBuilderJob>());
    }
    inactive_jobs.reserve(MAX_CHUNK_JOBS);

    root = nullptr;
    resetOctree(kDefaultMaxDepth, kDefaultVoxelSize);
}
TerrainOctreeImpl::~TerrainOctreeImpl() { destroyNode(root); }

void TerrainOctreeImpl::resetOctree(unsigned int _maxDepth, float _voxelSize) {
    maxDepth = _maxDepth;
    voxelSize = _voxelSize;

    if (root)
        destroyNode(root);

    const float root_extents = voxelSize * (1 << maxDepth);
    root = allocateNode(Vector3(0, 0, 0), root_extents, maxDepth);
}

void TerrainOctreeImpl::update(const Vector3& pointOfFocus) {
    LODSelector lodRequestor;
    lodRequestor.configure(pointOfFocus, voxelSize, maxDepth);
    updateLODsRecursive(root, lodRequestor);

    serveBuildRequests();
}

void TerrainOctreeImpl::updateLODsRecursive(TerrainNode* node,
                                            const LODSelector& lodRequestor) {
    // Nodes are boxes in 3D space, so a node can intersect multiple LOD
    // spheres. First get the smallest (highest detail) LOD sphere that the
    // node intersects.
    const unsigned int smallestRequestedLOD =
        lodRequestor.smallestLODInNode(*node);

    // If we have a leaf, and the requested LOD is < the node depth, we
    // divide the node and repeat.
    if (smallestRequestedLOD < node->depth) {
        if (node->isLeaf()) {
            divideNode(node);
        }
        assert(!node->isLeaf());
        for (int i = 0; i < 8; i++) {
            updateLODsRecursive(node->children[i], lodRequestor);
        }
    } else if (node->depth <= smallestRequestedLOD) {
        if (!node->isLeaf()) {
            // If we do not have a leaf, and the requested LOD is > the node
            // depth, we can merge the node
            mergeNode(node);
        }
    }
}

static void loadChunkJobData(ChunkBuilderJob& job,
                             const SDFGeneratorDelegate& generator,
                             const TerrainNode& chunk) {
    job.vertex_map.clear();
    job.border_triangles.clear();

    job.builder.reset();
    job.builder.addLayout(POSITION);
    job.builder.addLayout(NORMAL);

    // Load Sampled Data
    job.chunk_id = chunk.uniqueID;
    job.chunk_position =
        chunk.center - Vector3(chunk.extents, chunk.extents, chunk.extents);
    job.chunk_size = chunk.extents * 2;

    const float DISTANCE_BETWEEN_SAMPLES =
        job.chunk_size / (TERRAIN_SAMPLES_PER_CHUNK - 1);

    for (int i = 0; i < TERRAIN_SAMPLES_PER_CHUNK + 2; i++) {
        for (int j = 0; j < TERRAIN_SAMPLES_PER_CHUNK + 2; j++) {
            for (int k = 0; k < TERRAIN_SAMPLES_PER_CHUNK + 2; k++) {
                const float sample_x =
                    job.chunk_position.x + (i - 1) * DISTANCE_BETWEEN_SAMPLES;
                const float sample_y =
                    job.chunk_position.y + (j - 1) * DISTANCE_BETWEEN_SAMPLES;
                const float sample_z =
                    job.chunk_position.z + (k - 1) * DISTANCE_BETWEEN_SAMPLES;

                job.data[i][j][k] =
                    generator.sampleSDF(sample_x, sample_y, sample_z);
            }
        }
    }
}

void TerrainOctreeImpl::serveBuildRequests() {
    // Iterate through my existing jobs to see if any have finished. If they
    // have, load their meshes into the mesh pool.
    inactive_jobs.clear();

    for (int i = 0; i < jobs.size(); i++) {
        auto& job = jobs[i];
        if (job->async_lock.try_lock()) {
            if (job->status == ChunkBuilderJob::JobStatus::Done) {
                const unsigned int chunk_id = job->chunk_id;

                // There is a chance that the chunk was destroyed before the job
                // finished. In that case discard the contents of the job.
                // Otherwise, we will generate a mesh.
                if (auto iter = node_map.find(chunk_id);
                    iter != node_map.end()) {
                    iter->second->loaded = true;
                    iter->second->mesh =
                        resourceManager->requestMesh(job->builder);
                }

                job->status = ChunkBuilderJob::JobStatus::Inactive;
            }

            // There is a chance the async thread has not started yet at all, in
            // which case the status would be "active". So, we only want to kick
            // off a job if the status is inactive.
            if (job->status == ChunkBuilderJob::JobStatus::Inactive) {
                inactive_jobs.push_back(i);
            }

            job->async_lock.unlock();
        }
    }

    // For N dirty chunks, kick off new jobs for them.
    // TODO: Prioritize some dirty chunks over others.
    if (!inactive_jobs.empty() && !dirty_nodes.empty()) {
        const TerrainNodeID dirty_chunk = dirty_nodes[0];
        // TODO this is very inefficient and should be done better. However,
        // we want to process in order of what we place in the queue so that we
        // can see results faster.
        dirty_nodes.erase(dirty_nodes.begin());

        // There is a chance that the dirty chunk was destroyed before a job was
        // kicked off. Ignore it if so.
        if (auto iter = node_map.find(dirty_chunk); iter != node_map.end()) {
            auto& job = jobs[inactive_jobs.back()];
            inactive_jobs.pop_back();

            job->async_lock.lock();
            {
                loadChunkJobData(*job, *sdfGenerator, *iter->second);
                job->status = ChunkBuilderJob::JobStatus::Active;

                // Kick off thread
                ChunkBuilderJob* job_ptr = job.get();
                ThreadPool::GetThreadPool()->scheduleJob(
                    [job_ptr] { job_ptr->buildChunkMesh(); });
            }
            job->async_lock.unlock();
        }
    }
}

static void findValidMeshesHelper(TerrainNode* node,
                                  std::vector<Mesh*>& meshes) {
    if (node->isLeaf()) {
        if (node->mesh) {
            assert(node->mesh->ready);
            meshes.push_back(node->mesh.get());
        }
    } else {
        bool all_child_meshes_valid = true;

        for (int i = 0; i < 8; i++) {
            const auto& child = node->children[i];
            assert(child);

            if (!child->loaded) {
                all_child_meshes_valid = false;
            } else if (child->mesh && !child->mesh->ready) {
                all_child_meshes_valid = false;
            }
        }

        if (all_child_meshes_valid) {
            for (int i = 0; i < 8; i++) {
                findValidMeshesHelper(node->children[i], meshes);
            }
        } else {
            if (node->mesh) {
                assert(node->mesh->ready);
                meshes.push_back(node->mesh.get());
            }
        }
    }
}

void TerrainOctreeImpl::pullTerrainMeshes(std::vector<Mesh*>& meshes) {
    meshes.clear();
    if (root && root->loaded) {
        if (!root->mesh || root->mesh->ready) {
            findValidMeshesHelper(root, meshes);
        }
    }
}

TerrainNode* TerrainOctreeImpl::allocateNode(const Vector3& center,
                                             const float extents,
                                             unsigned int depth) {
    const unsigned int nodeID = idCounter++;
    TerrainNode* newNode = allocator.allocate();
    newNode->uniqueID = nodeID;

    newNode->loaded = false;
    newNode->mesh = nullptr;

    newNode->center = center;
    newNode->extents = extents;
    newNode->depth = depth;

    assert(!node_map.contains(nodeID));
    node_map[nodeID] = newNode;

    dirty_nodes.push_back(nodeID);

    return newNode;
}

void TerrainOctreeImpl::destroyNode(TerrainNode* node) {
    assert(node_map.contains(node->uniqueID));
    node_map.erase(node->uniqueID);

    allocator.free(node);
}

void TerrainOctreeImpl::divideNode(TerrainNode* node) {
    assert(node->isLeaf());
    if (node->isLeaf() && node->depth > 0) {
        // clang-format off
        const static Vector3 child_centers[8] = {
            Vector3(1, 1, 1),   Vector3(-1, 1, 1),
            Vector3(1, -1, 1),  Vector3(-1, -1, 1),
            Vector3(1, 1, -1),  Vector3(-1, 1, -1),
            Vector3(1, -1, -1), Vector3(-1, -1, -1),
        };
        // clang-format on
        const float child_extents = node->extents / 2;
        const unsigned int child_depth = node->depth - 1;

        for (int i = 0; i < 8; i++) {
            node->children[i] =
                allocateNode(node->center + child_centers[i] * child_extents,
                             child_extents, child_depth);
        }
    }
}

void TerrainOctreeImpl::mergeNode(TerrainNode* node) {
    // Do before so that we can get the children IDs
    if (!node->isLeaf()) {
        destroyAllChildren(node);
    }
}

void TerrainOctreeImpl::destroyAllChildren(TerrainNode* node) {
    if (!node->isLeaf()) {
        for (int i = 0; i < 8; i++) {
            destroyAllChildren(node->children[i]);
            destroyNode(node->children[i]);
            node->children[i] = nullptr;
        }
    }
}

} // namespace Graphics
} // namespace Engine