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
static constexpr uint8_t kTerrainChunkSlot = 5;
struct TerrainChunk {
    Vector2 position; // Bottom-Left (x,z) Coordinates
    Vector2 extents;
};

struct QuadTreeNode {
    TerrainChunk data;
    QuadTreeNode* children[4] = {nullptr};

    bool isLeaf() const { return children[0] == nullptr; }
};

class Terrain2DManagerImpl {
  private:
    struct Config {
        float lodAttenuation = 1000.f;

        int terrainMeshSampleCount = 15;

        Vector2 heightMapOrigin = Vector2(0, 0);
        Vector2 heightMapExtents = Vector2(1000, 1000);
        int heightmapNumSamples = 350;
    } config;

    // Settings
    static constexpr int kMaximumNodes = 5000;
    static constexpr int kMaxQuadTreeDepth = 10;
    static constexpr float kTerrainNodeSize = 25.f;

    VisualSystem* mVisualSystem;
    RenderManager* mRenderManager;

    std::unique_ptr<HeightMapGenerator> mHeightMap;

    std::shared_ptr<Mesh> mTerrainMesh;
    std::shared_ptr<Material> mTerrainMaterial;
    Technique* mTerrainTechnique;

    // QuadTree
    QuadTreeNode* root = nullptr;
    PoolAllocator<QuadTreeNode, kMaximumNodes> mQuadTreeAllocator;

    DrawBlockKey terrainDrawKey = kInvalidDrawBlockKey;
    std::vector<TerrainChunk> chunksToRender;

  public:
    Terrain2DManagerImpl(VisualSystem* visualSystem);
    ~Terrain2DManagerImpl();

    void update(const Vector3& cameraPosition);
    void imGui();
    void reset();

  private:
    void regenerateMesh();
    void regenerateHeightmapTexture();

    void updateQuadTreeRecursive(QuadTreeNode* node,
                                 const Vector3& cameraPosition, int depth);

    uint8_t computeIdealLOD(QuadTreeNode* node, const Vector3& cameraPosition);

    // QuadTree Management
    QuadTreeNode* allocateNode(const Vector2& position, const Vector2& extents);
    void destroyNode(QuadTreeNode* node);
    void divideNode(QuadTreeNode& node);
    void mergeNode(QuadTreeNode& node);
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

    // Because our terrain is heightmap based, we can use a single mesh and
    // instance draw it for each chunk, reading from heightmap texture for the
    // height.
    mTerrainMaterial = visualSystem->getMaterialManager()->createMaterial(
        MaterialManager::TerrainMaterialParams());
    regenerateMesh();
    regenerateHeightmapTexture();

    reset();

    ImGuiHelper::registerImGuiCallback("Render/Terrain2D",
                                       [this]() { imGui(); });
}
Terrain2DManagerImpl::~Terrain2DManagerImpl() = default;

void Terrain2DManagerImpl::update(const Vector3& cameraPosition) {
    chunksToRender.clear();
    updateQuadTreeRecursive(root, cameraPosition, 0);

    const bool render = !chunksToRender.empty() && mTerrainMesh->ready &&
                        mTerrainMaterial->ready();
    if (render) {
        if (terrainDrawKey == kInvalidDrawBlockKey) {
            DrawBlock block;
            block.initialize(AABB(), mTerrainMesh.get(),
                             mTerrainMaterial.get());
            block.numInstances = chunksToRender.size();
            terrainDrawKey = mRenderManager->addDrawBlock(block);
        } else {
            mRenderManager->updateInstanceData(terrainDrawKey, InstanceData(),
                                               chunksToRender.size());
        }

        mTerrainTechnique->clearVertexCB(kTerrainChunkSlot);

        const Vector2 heightMapPosition =
            config.heightMapOrigin - config.heightMapExtents / 2;
        mTerrainTechnique->uploadVertexCBData(
            kTerrainChunkSlot, &heightMapPosition, sizeof(Vector2));
        mTerrainTechnique->uploadVertexCBData(
            kTerrainChunkSlot, &config.heightMapExtents, sizeof(Vector2));

        mTerrainTechnique->uploadVertexCBData(
            kTerrainChunkSlot, chunksToRender.data(),
            chunksToRender.size() * sizeof(TerrainChunk));
        static_assert(sizeof(TerrainChunk) == sizeof(float) * 4);
    } else {
        if (terrainDrawKey != kInvalidDrawBlockKey) {
            mRenderManager->removeDrawBlock(terrainDrawKey);
            terrainDrawKey = kInvalidDrawBlockKey;
        }
    }
}

void Terrain2DManagerImpl::imGui() {
#if defined(IMGUI_ENABLED)
    ImGui::Text("# Chunks: %zu", mQuadTreeAllocator.getNumAllocations());
    ImGui::Text("# Leaves: %i", chunksToRender.size());

    ImGui::SliderFloat("LOD Attenuation", &config.lodAttenuation, 0.0, 10000.f);

    if (ImGui::CollapsingHeader("Terrain Mesh")) {
        ImGui::SliderInt("# Terrain Mesh Samples: %i",
                         &config.terrainMeshSampleCount, 2, 25);
        if (ImGui::Button("Reset Terrain Mesh")) {
            regenerateMesh();
        }
    }

    if (ImGui::CollapsingHeader("Height Map Settings")) {
        ImGui::SliderFloat2("Heightmap Origin:", &config.heightMapOrigin.x,
                            -500, 500);
        ImGui::SliderFloat2("Heightmap Extents:", &config.heightMapExtents.x,
                            10, 1000);
        ImGui::SliderInt("Heightmap Samples:", &config.heightmapNumSamples, 10,
                         1000);

        if (ImGui::Button("Reset Heightmap")) {
            regenerateHeightmapTexture();
        }
    }

    if (ImGui::CollapsingHeader("Noise Settings")) {
        mHeightMap->imGui();

        if (ImGui::Button("Reset")) {
            reset();
        }
    }
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

void Terrain2DManagerImpl::regenerateMesh() {
    const int numSamples = config.terrainMeshSampleCount;

    // Must be at least 2 so we can create a flat square.
    assert(numSamples >= 2);

    // Generate a very simple mesh within bounds
    // x,z in [0,1]
    // y = 0
    // TODO Add skirts for y < 0 so it's harder to see the gap between LODs
    MeshBuilder builder;

    builder.reset();
    builder.addLayout(POSITION);

    const float sampleDistanceInv = 1 / float(numSamples - 1);
    for (int sampleX = 0; sampleX < numSamples; sampleX++) {
        for (int sampleZ = 0; sampleZ < numSamples; sampleZ++) {
            const float x = sampleX * sampleDistanceInv;
            const float y = 0.f;
            const float z = sampleZ * sampleDistanceInv;
            builder.addVertex(Vector3(x, y, z));
        }
    }

    for (int indexX = 0; indexX < numSamples - 1; indexX++) {
        for (int indexZ = 0; indexZ < numSamples - 1; indexZ++) {
            // d c
            // a b
            const unsigned int a = indexZ + indexX * numSamples;
            const unsigned int b = indexZ + (indexX + 1) * numSamples;
            const unsigned int c = (indexZ + 1) + (indexX + 1) * numSamples;
            const unsigned int d = (indexZ + 1) + indexX * numSamples;
            builder.addTriangle(a, d, b);
            builder.addTriangle(c, b, d);
        }
    }

    mTerrainMesh = mVisualSystem->getResourceManager()->requestMesh(builder);
}

void Terrain2DManagerImpl::regenerateHeightmapTexture() {
    const int numSamples = config.heightmapNumSamples;

    TextureBuilder builder(numSamples, numSamples, TextureLayout::R32_FLOAT);

    TextureColor texel;
    auto& height = texel.asType<TextureColor::FloatR32>();

    const Vector2 heightMapPosition =
        config.heightMapOrigin - config.heightMapExtents / 2;
    const float distBetweenSamplesInv = 1 / float(numSamples - 1);
    for (int x = 0; x < numSamples; x++) {
        for (int z = 0; z < numSamples; z++) {
            const float worldX =
                heightMapPosition.x +
                x * distBetweenSamplesInv * config.heightMapExtents.x;
            const float worldZ =
                heightMapPosition.y +
                z * distBetweenSamplesInv * config.heightMapExtents.y;

            height.r = mHeightMap->sampleHeight(worldX, worldZ);
            builder.setColor(x, z, texel);
        }
    }

    std::shared_ptr<Texture> texture =
        mVisualSystem->getResourceManager()->requestTexture(builder, true);
    mTerrainTechnique = mTerrainMaterial->getTechnique(RenderPass::kOpaque);
    mTerrainTechnique->bindVertexShaderResource(0, texture,
                                                TextureSampler::Point);
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

    const uint8_t lod =
        kMaxQuadTreeDepth / (1 + distance / config.lodAttenuation);
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

    if (node->isLeaf()) {
        chunksToRender.push_back(node->data);
    }
}

QuadTreeNode* Terrain2DManagerImpl::allocateNode(const Vector2& position,
                                                 const Vector2& extents) {
    QuadTreeNode* node = mQuadTreeAllocator.allocate();
    node->data.position = position;
    node->data.extents = extents;

    return node;
}
void Terrain2DManagerImpl::destroyNode(QuadTreeNode* node) {
    const size_t index = mQuadTreeAllocator.getIndex(node);

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

} // namespace Graphics
} // namespace Engine