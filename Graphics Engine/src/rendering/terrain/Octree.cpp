#include "Octree.h"

#include <assert.h>

#include "core/ThreadPool.h"

#include "rendering/resources/ResourceManager.h"

#include "../VisualDebug.h"
#include "TerrainGeneration.h"

constexpr int MAX_CHUNK_JOBS = 16;

namespace Engine {
namespace Graphics {
TerrainNode::TerrainNode() : children{nullptr} {
    octree = nullptr;
    uniqueID = INVALID_NODE_ID;
}
TerrainNode::~TerrainNode() = default;

void TerrainNode::initialize(const Vector3& _center, float _extents,
                             unsigned int _depth) {
    center = _center;
    extents = _extents;
    depth = _depth;
}

bool TerrainNode::isLeaf() const { return children[0] == nullptr; }

void TerrainNode::divide() {
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
    }
}

void TerrainNode::merge() {
    // Do before so that we can get the children IDs
    if (!isLeaf()) {
        destroyAllChildren();
    }
}
void TerrainNode::destroyAllChildren() {
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

unsigned int OctreeUpdater::smallestLODInNode(const TerrainNode& node) const {
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

TerrainMeshLoader::TerrainMeshLoader(unsigned int _maxDepth, float _voxelSize)
    : config{_maxDepth, _voxelSize} {
    idCounter = INVALID_NODE_ID + 1;

    // Create MAX_CHUNK_JOBS schunk update jobs.
    for (int i = 0; i < MAX_CHUNK_JOBS; i++) {
        jobs.push_back(std::make_unique<ChunkBuilderJob>());
    }
    inactive_jobs.reserve(MAX_CHUNK_JOBS);

    const float root_extents = config.voxelSize * (1 << config.maxDepth);
    root = allocateNode();
    root->initialize(Vector3(0, 0, 0), root_extents, config.maxDepth);
}
TerrainMeshLoader::~TerrainMeshLoader() { destroyNode(root); }

TerrainNode* TerrainMeshLoader::allocateNode() {
    const unsigned int nodeID = idCounter++;
    TerrainNode* newNode = allocator.allocate();
    newNode->octree = this;
    newNode->uniqueID = nodeID;

    newNode->loaded = false;
    newNode->mesh = nullptr;

    assert(!node_map.contains(nodeID));
    node_map[nodeID] = newNode;

    dirty_nodes.push_back(nodeID);

    return newNode;
}

void TerrainMeshLoader::destroyNode(TerrainNode* node) {
    assert(node_map.contains(node->uniqueID));
    node_map.erase(node->uniqueID);

    allocator.free(node);
}

void TerrainMeshLoader::resetOctree(unsigned int _maxDepth, float _voxelSize) {
    config = {_maxDepth, _voxelSize};

    delete root;

    const float root_extents = config.voxelSize * (1 << config.maxDepth);
    root = allocateNode();
    root->initialize(Vector3(0, 0, 0), root_extents, config.maxDepth);
}
void TerrainMeshLoader::updateOctree(const OctreeUpdater& lodRequestor) {
    updateOctreeHelper(root, lodRequestor);
}
void TerrainMeshLoader::updateOctreeHelper(TerrainNode* node,
                                           const OctreeUpdater& lodRequestor) {
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
            updateOctreeHelper(node->children[i], lodRequestor);
        }
    } else if (node->depth <= smallestRequestedLOD) {
        if (!node->isLeaf()) {
            // If we do not have a leaf, and the requested LOD is > the node
            // depth, we can merge the node
            node->merge();
        }
    }
}

static void loadChunkJobData(ChunkBuilderJob& job,
                             const TerrainGeneration& generator,
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

                job.data[i][j][k] = generator.sampleTerrainGenerator(
                    sample_x, sample_y, sample_z);
            }
        }
    }
}

void TerrainMeshLoader::serveBuildRequests(TerrainGeneration* generator,
                                           ResourceManager* resource_manager) {
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
                        resource_manager->requestMesh(job->builder);
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
                loadChunkJobData(*job, *generator, *iter->second);
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

void TerrainMeshLoader::findValidMeshes(std::vector<Mesh*>& meshes) {
    meshes.clear();
    if (root && root->loaded) {
        if (!root->mesh || root->mesh->ready) {
            findValidMeshesHelper(root, meshes);
        }
    }
}

void TerrainMeshLoader::findValidMeshesHelper(TerrainNode* node,
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

OctreeUpdater TerrainMeshLoader::getUpdater() {
    return std::move(OctreeUpdater(config.maxDepth));
}

const std::unordered_map<TerrainNodeID, TerrainNode*>&
TerrainMeshLoader::getNodeMap() const {
    return node_map;
}

const TerrainNode* TerrainMeshLoader::getNode(TerrainNodeID id) const {
    if (node_map.contains(id))
        return node_map.at(id);
    else
        return nullptr;
}

bool TerrainMeshLoader::isNodePresent(TerrainNodeID id) const {
    return node_map.contains(id);
}

bool TerrainMeshLoader::isNodeLeaf(const TerrainNodeID id) const {
    if (isNodePresent(id)) {
        return node_map.at(id)->isLeaf();
    } else {
        return false;
    }
}

} // namespace Graphics
} // namespace Engine