#include "Terrain2DManager.h"

#include <assert.h>
#include <vector>

#include "core/PoolAllocator.h"
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

        // Mesh Generation Settings
        int terrainMeshSampleCount = 15;
        bool generateSkirt = true;
        float skirtDepth = 25.f;

        // Heightmap Generation Settings
        Vector2 heightMapOrigin = Vector2(0, 0);
        Vector2 heightMapExtents = Vector2(2500, 2500);
        int heightmapNumSamples = 450;
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
        ImGui::Checkbox("Generate Skirt", &config.generateSkirt);
        if (config.generateSkirt) {
            ImGui::SliderFloat("Terrain Skirt Depth:", &config.skirtDepth, 0.f,
                               50.f);
        }

        if (ImGui::Button("Reset Terrain Mesh")) {
            regenerateMesh();
        }
    }

    if (ImGui::CollapsingHeader("Height Map Settings")) {
        ImGui::SliderFloat2("Heightmap Origin:", &config.heightMapOrigin.x,
                            -500, 500);
        ImGui::SliderFloat2("Heightmap Extents:", &config.heightMapExtents.x,
                            10, 5000);
        ImGui::SliderInt("Heightmap Samples:", &config.heightmapNumSamples, 10,
                         2500);

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

    // Generate a grid of points in the order of
    // 6 7 8...
    // 3 4 5
    // 0 1 2
    // Bottom left corner is (x,z) = (0,0). Right is +x, Up is +z.
    const float sampleDistanceInv = 1 / float(numSamples - 1);
    for (int sampleX = 0; sampleX < numSamples; sampleX++) {
        for (int sampleZ = 0; sampleZ < numSamples; sampleZ++) {
            const float x = sampleX * sampleDistanceInv;
            const float y = 0.f;
            const float z = sampleZ * sampleDistanceInv;
            builder.addVertex(Vector3(x, y, z));
        }
    }

    // Connect my points together. For a given quad in the grid, a,b,c,d
    // reference indices as so
    // d c
    // a b
    for (int indexX = 0; indexX < numSamples - 1; indexX++) {
        for (int indexZ = 0; indexZ < numSamples - 1; indexZ++) {
            const unsigned int a = indexZ + indexX * numSamples;
            const unsigned int b = indexZ + (indexX + 1) * numSamples;
            const unsigned int c = (indexZ + 1) + (indexX + 1) * numSamples;
            const unsigned int d = (indexZ + 1) + indexX * numSamples;
            builder.addTriangle(a, d, b);
            builder.addTriangle(c, b, d);
        }
    }

    // Generate a skirt. This is a set of vertices that protrude downwards from
    // the edges of the terrain mesh. Skirts are a cheap and simple way to hide
    // the LOD transitions.
    if (config.generateSkirt) {
        const unsigned int skirtIndexStart = builder.getVertices().size();

        std::vector<unsigned int> borderIndices;
        auto generateSkirtVertices = [&builder, &borderIndices, &numSamples,
                                      this](unsigned int startX,
                                            unsigned int startZ, int offsetX,
                                            int offsetZ) {
            for (int i = 0; i < numSamples; i++) {
                const unsigned int indexX = startX + offsetX * i;
                const unsigned int indexZ = startZ + offsetZ * i;

                const unsigned int vertexIndex = indexZ + indexX * numSamples;
                borderIndices.push_back(vertexIndex);

                const Vector3 skirtVertex =
                    builder.getVertex(vertexIndex).position +
                    Vector3(0, -config.skirtDepth, 0);
                builder.addVertex(skirtVertex);
            }
        };

        // Walk the grid border counter-clockwise and generate the skirt
        // vertices
        generateSkirtVertices(0, 0, 1, 0);
        generateSkirtVertices(numSamples - 1, 0, 0, 1);
        generateSkirtVertices(numSamples - 1, numSamples - 1, -1, 0);
        generateSkirtVertices(0, numSamples - 1, 0, -1);

        assert(builder.getVertices().size() - skirtIndexStart ==
               borderIndices.size());
        for (int i = 0; i < borderIndices.size() - 1; i++) {
            unsigned int a = borderIndices[i];
            unsigned int b = borderIndices[i + 1];
            unsigned int c = skirtIndexStart + i + 1;
            unsigned int d = skirtIndexStart + i;

            builder.addTriangle(a, b, d);
            builder.addTriangle(c, d, b);
        };
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
                                                SamplerType::Sampler_Point);
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