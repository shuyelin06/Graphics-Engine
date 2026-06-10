#include "Terrain2DManager.h"

#include <array>
#include <assert.h>
#include <atomic>
#include <bitset>
#include <vector>

#include "core/PoolAllocator.h"
#include "core/ThreadPool.h"
#include "math/Triangle.h"
#include "math/Vector2.h"

#include "rendering/VisualSystem.h"
#include "rendering/pipeline/RenderManager.h"
#include "rendering/resources/MaterialManager.h"
#include "rendering/resources/ResourceManager.h"

#include "rendering/ImGui.h"

#include "HeightMapGenerator.h"

namespace Engine {
namespace Graphics {
using TerrainChunkID = uint32_t;
static constexpr TerrainChunkID kInvalidNodeID = 0xFFFF;
struct TerrainChunk {
    TerrainChunkID id = kInvalidNodeID;

    Vector2 position; // Bottom-Left (x,z) Coordinates
    Vector2 extents;

    // Rendering
    std::shared_ptr<Mesh> mesh = nullptr;
    DrawBlockKey blockKey = kInvalidDrawBlockKey;

    bool isMeshReady() const { return mesh && mesh->ready; }
};

struct QuadTreeNode {
    TerrainChunk data;
    QuadTreeNode* children[4] = {nullptr};

    bool isLeaf() const { return children[0] == nullptr; }
};

class Terrain2DManagerImpl {
  private:
    // Settings
    static constexpr int kMaximumNodes = 5000;
    static constexpr int kMaxQuadTreeDepth = 10;
    static constexpr float kTerrainNodeSize = 25.f;
    static constexpr int kNumSamples = 10;
    static constexpr int kNumBuilderJobs = 8;
    float kLODAttenuation = 3000.f;

    VisualSystem* mVisualSystem;
    RenderManager* mRenderManager;

    std::unique_ptr<HeightMapGenerator> mHeightMap;
    std::shared_ptr<Material> mTerrainMaterial;

    // QuadTree
    QuadTreeNode* root = nullptr;
    PoolAllocator<QuadTreeNode, kMaximumNodes> mQuadTreeAllocator;
    std::bitset<kMaximumNodes> mAllocationTracker;
    uint32_t mIDCounter;

    // Terrain Mesh Building
    struct MeshBuilderJob {
        std::atomic<bool> loading = false;

        // Input
        TerrainChunk chunkData{};
        QuadTreeNode* ptr = nullptr;

        // Output
        MeshBuilder output{};
        bool hasOutput = false;
    };
    std::array<MeshBuilderJob, kNumBuilderJobs> mMeshBuilderJobs;
    std::vector<std::pair<TerrainChunkID, QuadTreeNode*>> mDirtyChunks;

  public:
    Terrain2DManagerImpl(VisualSystem* visualSystem);
    ~Terrain2DManagerImpl();

    void update(const Vector3& cameraPosition);
    void imGui();
    void reset();

  private:
    void processMeshBuilds();
    void updateQuadTreeRecursive(QuadTreeNode* node,
                                 const Vector3& cameraPosition, int depth);
    void selectQuadTreeMeshes(QuadTreeNode* node);

    uint8_t computeIdealLOD(QuadTreeNode* node, const Vector3& cameraPosition);

    // QuadTree Management
    QuadTreeNode* allocateNode(const Vector2& position, const Vector2& extents);
    void destroyNode(QuadTreeNode* node);
    void divideNode(QuadTreeNode& node);
    void mergeNode(QuadTreeNode& node);

    void buildTerrainMesh(MeshBuilderJob& job);
};

std::unique_ptr<Terrain2DManager>
Terrain2DManager::create(VisualSystem* visualSystem) {
    std::unique_ptr<Terrain2DManager> ptr =
        std::unique_ptr<Terrain2DManager>(new Terrain2DManager());
    ptr->mImpl = std::make_unique<Terrain2DManagerImpl>(visualSystem);
    return ptr;
}

Terrain2DManager::Terrain2DManager() = default;
Terrain2DManager::~Terrain2DManager() = default;

void Terrain2DManager::update(const Vector3& cameraPosition) {
    mImpl->update(cameraPosition);
}

void Terrain2DManager::imGui() { mImpl->imGui(); }

Terrain2DManagerImpl::Terrain2DManagerImpl(VisualSystem* visualSystem)
    : mVisualSystem(visualSystem) {
    mRenderManager = mVisualSystem->getRenderManager();
    mHeightMap = std::make_unique<HeightMapGenerator>();

    mIDCounter = 0;

    MaterialManager::TerrainMaterialParams terrainParams{};
    mTerrainMaterial =
        visualSystem->getMaterialManager()->createMaterial(terrainParams);

    reset();
}
Terrain2DManagerImpl::~Terrain2DManagerImpl() = default;

void Terrain2DManagerImpl::update(const Vector3& cameraPosition) {
    processMeshBuilds();

    updateQuadTreeRecursive(root, cameraPosition, 0);

    if (root->data.isMeshReady())
        selectQuadTreeMeshes(root);
}

void Terrain2DManagerImpl::imGui() {
#if defined(IMGUI_ENABLED)
    ImGui::Text("# Chunks: %zu", mQuadTreeAllocator.getNumAllocations());

    if (ImGui::CollapsingHeader("Generation Settings")) {
        mHeightMap->imGui();
        
        if (ImGui::Button("Reset")) {
            reset();
        }
    }

    if (ImGui::CollapsingHeader("Chunk Loading")) {
        ImGui::Text("# Dirty Chunks: %zu", mDirtyChunks.size());
        ImGui::Text("# Active Jobs: %zu",
                    kNumBuilderJobs - mMeshBuilderJobs.size());
    }

    ImGui::SliderFloat("LOD Attenuation", &kLODAttenuation, 0.0, 10000.f);
    ImGui::Text("Terain2D");
#endif
}

void Terrain2DManagerImpl::reset() {
    if (root) {
        destroyNode(root);
        root = nullptr;
    }

    const float rootSize = kTerrainNodeSize * (1 << kMaxQuadTreeDepth);
    root = allocateNode(Vector2(-rootSize / 2, -rootSize / 2),
                        Vector2(rootSize, rootSize));
}

void Terrain2DManagerImpl::processMeshBuilds() {
    std::vector<uint8_t> inactiveJobs;

    auto getNodeIfValid = [this](QuadTreeNode* ptr, TerrainChunkID id) {
        const size_t index = mQuadTreeAllocator.getIndex(ptr);
        QuadTreeNode* output = ptr;
        if (!mAllocationTracker.test(index) || ptr->data.id != id) {
            output = nullptr;
        }
        return output;
    };

    // Serve builder jobs
    for (int i = 0; i < mMeshBuilderJobs.size(); i++) {
        MeshBuilderJob& job = mMeshBuilderJobs[i];
        if (job.loading)
            continue;

        if (job.hasOutput) {
            QuadTreeNode* node = getNodeIfValid(job.ptr, job.chunkData.id);
            if (node) {
                assert(!node->data.mesh);
                node->data.mesh =
                    mVisualSystem->getResourceManager()->requestMesh(
                        job.output);
            }

            job.hasOutput = false;
        }

        inactiveJobs.push_back(i);
    }

    // Pull from the dirty chunks and kick off more jobs
    while (!mDirtyChunks.empty() && !inactiveJobs.empty()) {
        const uint8_t inactiveJobIndex = inactiveJobs.back();

        const auto dirtyChunk = mDirtyChunks.back();
        mDirtyChunks.pop_back();
        QuadTreeNode* node =
            getNodeIfValid(dirtyChunk.second, dirtyChunk.first);

        if (node) {
            // Set up job
            inactiveJobs.pop_back();

            MeshBuilderJob& job = mMeshBuilderJobs[inactiveJobIndex];
            job.chunkData = node->data;
            job.ptr = node;

            job.loading = true;
            ThreadPool::GetThreadPool()->scheduleJob([this, &job] {
                buildTerrainMesh(job);
                job.loading = false;
            });
        }
    }
}

uint8_t Terrain2DManagerImpl::computeIdealLOD(QuadTreeNode* node,
                                              const Vector3& cameraPosition) {
    // Find distance from node to camera. If node is in the camera, distance is
    // 0.
    const Vector2 halfExtents = node->data.extents / 2;
    const Vector2 center = node->data.position + halfExtents;

    Vector2 relPos = cameraPosition.xz() - center;
    relPos.x = abs(relPos.x);
    relPos.y = abs(relPos.y);

    relPos.x = max(relPos.x - halfExtents.x, 0.f);
    relPos.y = max(relPos.y - halfExtents.y, 0.f);

    const float distance = relPos.magnitude();

    const uint8_t lod = kMaxQuadTreeDepth / (1 + distance / kLODAttenuation);
    if (lod > kMaxQuadTreeDepth)
        return kMaxQuadTreeDepth;
    else
        return lod;
}

void Terrain2DManagerImpl::updateQuadTreeRecursive(
    QuadTreeNode* node, const Vector3& cameraPosition, int depth) {
    const uint8_t idealLOD = computeIdealLOD(node, cameraPosition);
    if (node->isLeaf()) {
        if (idealLOD > depth) {
            divideNode(*node);

            for (int i = 0; i < 4; i++) {
                updateQuadTreeRecursive(node->children[i], cameraPosition,
                                        depth + 1);
            }
        }
    } else {
        if (idealLOD < depth) {
            mergeNode(*node);
        } else {
            for (int i = 0; i < 4; i++) {
                updateQuadTreeRecursive(node->children[i], cameraPosition,
                                        depth + 1);
            }
        }
    }
}

void Terrain2DManagerImpl::selectQuadTreeMeshes(QuadTreeNode* node) {
    assert(node->data.isMeshReady());
    if (node->isLeaf()) {
        if (node->data.blockKey == kInvalidDrawBlockKey) {
            DrawBlock drawBlock;
            drawBlock.initialize(node->data.mesh->aabb, node->data.mesh.get(),
                                 mTerrainMaterial.get());
            node->data.blockKey = mRenderManager->addDrawBlock(drawBlock);
        }
    } else {
        bool childrenReady = true;

        for (int i = 0; i < 4; i++) {
            QuadTreeNode* child = node->children[i];
            childrenReady = childrenReady && child->data.isMeshReady();
        }

        if (childrenReady) {
            if (node->data.blockKey != kInvalidDrawBlockKey) {
                mRenderManager->removeDrawBlock(node->data.blockKey);
                node->data.blockKey = kInvalidDrawBlockKey;
            }

            for (int i = 0; i < 4; i++) {
                selectQuadTreeMeshes(node->children[i]);
            }
        }
    }
}

QuadTreeNode* Terrain2DManagerImpl::allocateNode(const Vector2& position,
                                                 const Vector2& extents) {
    QuadTreeNode* node = mQuadTreeAllocator.allocate();
    node->data.id = mIDCounter++;
    node->data.position = position;
    node->data.extents = extents;
    node->data.blockKey = kInvalidDrawBlockKey;
    node->data.mesh = nullptr;

    mDirtyChunks.emplace_back(node->data.id, node);

    const size_t index = mQuadTreeAllocator.getIndex(node);
    mAllocationTracker.set(index, true);

    return node;
}
void Terrain2DManagerImpl::destroyNode(QuadTreeNode* node) {
    if (node->data.blockKey != kInvalidDrawBlockKey) {
        mRenderManager->removeDrawBlock(node->data.blockKey);
        node->data.blockKey = kInvalidDrawBlockKey;
    }

    const size_t index = mQuadTreeAllocator.getIndex(node);
    mAllocationTracker.set(index, true);

    if (!node->isLeaf()) {
        for (int i = 0; i < 4; i++) {
            destroyNode(node->children[i]);
        }
    }
    mQuadTreeAllocator.free(node);
}

void Terrain2DManagerImpl::divideNode(QuadTreeNode& node) {
    assert(node.isLeaf());
    const auto& data = node.data;
    const Vector2 halfExtents = data.extents / 2;

    // Allocated in this order (bottom-left is parent position (x,z))
    // C D
    // A B
    node.children[0] = allocateNode(data.position, halfExtents);
    node.children[1] =
        allocateNode(data.position + Vector2(halfExtents.x, 0), halfExtents);
    node.children[2] =
        allocateNode(data.position + Vector2(0, halfExtents.y), halfExtents);
    node.children[3] = allocateNode(data.position + halfExtents, halfExtents);
}

void Terrain2DManagerImpl::mergeNode(QuadTreeNode& node) {
    assert(!node.isLeaf());

    for (int i = 0; i < 4; i++) {
        QuadTreeNode* child = node.children[i];
        destroyNode(child);
        node.children[i] = nullptr;
    }
}

inline static unsigned int
addVertexToBuilder(std::unordered_map<Vector3, unsigned int>& vertexMap,
                   MeshBuilder& builder, const Vector3& v) {
    unsigned int i;
    if (vertexMap.contains(v)) {
        i = vertexMap[v];
    } else {
        i = builder.addVertex(v);
        vertexMap[v] = i;
    }
    return i;
}
inline static void
addTriangleToBuilder(std::unordered_map<Vector3, unsigned int>& vertexMap,
                     MeshBuilder& builder, const Vector3& a, const Vector3& b,
                     const Vector3& c) {
    builder.addTriangle(addVertexToBuilder(vertexMap, builder, a),
                        addVertexToBuilder(vertexMap, builder, b),
                        addVertexToBuilder(vertexMap, builder, c));
}

void Terrain2DManagerImpl::buildTerrainMesh(MeshBuilderJob& job) {
    auto indexToWorldPosition = [&job](int xIndex, int zIndex) {
        const TerrainChunk& chunk = job.chunkData;
        // Local Space [0,1]
        constexpr int indexRange = kNumSamples - 1;
        const float xLocal = float(indexRange - xIndex) / float(indexRange);
        const float zLocal = float(indexRange - zIndex) / float(indexRange);
        // World Space [Position, Position + Extents]
        float xWorld = chunk.position.x + chunk.extents.x * xLocal;
        float zWorld = chunk.position.y + chunk.extents.y * zLocal;

        return Vector2(xWorld, zWorld);
    };

    // Step 2: Generate triangles using heightmap data
    MeshBuilder& builder = job.output;

    std::unordered_map<Vector3, unsigned int> vertexMap;
    builder.reset();
    builder.addLayout(POSITION);
    builder.addLayout(NORMAL);

    std::vector<Triangle> borderTriangles;
    for (int indexX = -1; indexX < kNumSamples + 1; indexX++) {
        for (int indexZ = -1; indexZ < kNumSamples + 1; indexZ++) {
            const Vector2 pos00 = indexToWorldPosition(indexX, indexZ);
            const Vector2 pos10 = indexToWorldPosition(indexX + 1, indexZ);
            const Vector2 pos01 = indexToWorldPosition(indexX, indexZ + 1);
            const Vector2 pos11 = indexToWorldPosition(indexX + 1, indexZ + 1);

            const float height00 = mHeightMap->sampleHeight(pos00);
            const float height10 = mHeightMap->sampleHeight(pos10);
            const float height01 = mHeightMap->sampleHeight(pos01);
            const float height11 = mHeightMap->sampleHeight(pos11);

            const Vector3 worldPos00 = Vector3(pos00, height00).xzy();
            const Vector3 worldPos10 = Vector3(pos10, height10).xzy();
            const Vector3 worldPos01 = Vector3(pos01, height01).xzy();
            const Vector3 worldPos11 = Vector3(pos11, height11).xzy();

            const bool isBorderTriangle = indexX == -1 || indexZ == -1 ||
                                          indexX == kNumSamples ||
                                          indexZ == kNumSamples;
            if (isBorderTriangle) {
                borderTriangles.push_back(
                    Triangle(worldPos00, worldPos01, worldPos10));
                borderTriangles.push_back(
                    Triangle(worldPos11, worldPos10, worldPos01));
            } else {
                addTriangleToBuilder(vertexMap, builder, worldPos00, worldPos01,
                                     worldPos10);
                addTriangleToBuilder(vertexMap, builder, worldPos11, worldPos10,
                                     worldPos01);
            }
        }
    }

    for (const Triangle& tri : borderTriangles) {
        addTriangleToBuilder(vertexMap, builder, tri.vertex(0), tri.vertex(1),
                             tri.vertex(2));
    }
    builder.regenerateNormals();
    builder.popTriangles(borderTriangles.size());

    // Done. Change job status
    assert(!builder.isEmpty());
    job.hasOutput = !builder.isEmpty();
}

} // namespace Graphics
} // namespace Engine