#include "Terrain2DManager.h"

#include <assert.h>
#include <vector>

#include "core/PoolAllocator.h"
#include "math/Vector2.h"

#include "rendering/VisualSystem.h"
#include "rendering/core/Device.h"
#include "rendering/pipeline/RenderManager.h"
#include "rendering/resources/MaterialManager.h"
#include "rendering/resources/ResourceManager.h"

#include "rendering/ImGui.h"
#include "util/Profiling.h"

#include "HeightMapGenerator.h"
#include "TerrainMaterialStore.h"
#include "TerrainUtil.h"

namespace Engine
{
namespace Graphics
{
static constexpr uint8_t kTerrainChunkSlot = 5;
struct TerrainChunk
{
    Vector2 position; // Bottom-Left (x,z) Coordinates
    Vector2 extents;
};

struct QuadTreeNode
{
    TerrainChunk data;
    DrawBlockKey blockKey = kInvalidDrawBlockKey;
    QuadTreeNode* children[4] = {nullptr};

    bool isLeaf() const { return children[0] == nullptr; }
};

class Terrain2DManagerImpl
{
  private:
    struct Config
    {
        float lodAttenuation = 1000.f;

        // Heightmap Generation Settings
        bool invalidateHeightmap = false;
        Vector2 heightMapOrigin = Vector2(0, 0);
        Vector2 heightMapExtents = Vector2(2500, 2500);
    } config;
    struct ShaderSettings
    {
        float triplanarTextureScale = 0.005f;
        float triplanarTextureSharpness = 1.f;
        float noiseScaling = 1.f;
        float pad1;
    } shaderSettings;

    // Settings
    static constexpr int kMaximumNodes = 32;
    static constexpr int kMaxQuadTreeDepth = 10;
    static constexpr float kTerrainNodeSize = 25.f;

    VisualSystem* mVisualSystem;
    RenderManager* mRenderManager;

    std::unique_ptr<HeightMapGenerator> mHeightmapGenerator;
    std::unique_ptr<TerrainMaterialStore> mMaterialStore;

    std::shared_ptr<Geometry> mTerrainMesh;
    std::shared_ptr<Material> mTerrainMaterial;

    Technique* mTerrainTechnique;

    // QuadTree
    QuadTreeNode* root = nullptr;
    PoolAllocator<QuadTreeNode, kMaximumNodes> mQuadTreeAllocator;

    std::vector<TerrainChunk> chunksToRender;

  public:
    Terrain2DManagerImpl(VisualSystem* visualSystem);
    ~Terrain2DManagerImpl();

    void updatePerform(const Vector3& cameraPosition, DeviceContext* context);

    void imGui();
    void reset();

  private:
    void setupTerrainMesh(int numSamples, float skirtDepth);
    void setupTerrainMaterial();

    void updateIdealQuadTreeLOD(QuadTreeNode* node,
                                const Vector3& cameraPosition,
                                int depth);

    void selectReadyQuadTreeNodes();
    bool nodeReadyCheck(QuadTreeNode* node);
    void selectReadyQuadTreeNodesHelper(QuadTreeNode* node);

    void submitTerrainForRendering();

    void invalidateHeightmap();
    void regenerateHeightmapTexture(DeviceContext* context);

    uint8_t computeIdealLOD(QuadTreeNode* node, const Vector3& cameraPosition);

    // QuadTree Management
    QuadTreeNode* allocateNode(const Vector2& position, const Vector2& extents);
    void destroyNode(QuadTreeNode* node);
    void divideNode(QuadTreeNode& node);
    void mergeNode(QuadTreeNode& node);
};

std::unique_ptr<Terrain2DManager>
Terrain2DManager::create(VisualSystem* visualSystem)
{
    std::unique_ptr<Terrain2DManager> ptr =
        std::unique_ptr<Terrain2DManager>(new Terrain2DManager());
    ptr->mImpl = std::make_unique<Terrain2DManagerImpl>(visualSystem);
    return ptr;
}

Terrain2DManager::Terrain2DManager() = default;
Terrain2DManager::~Terrain2DManager() = default;

void Terrain2DManager::updatePerform(const Vector3& cameraPosition,
                                     DeviceContext* context)
{
    mImpl->updatePerform(cameraPosition, context);
}

void Terrain2DManager::imGui() { mImpl->imGui(); }

Terrain2DManagerImpl::Terrain2DManagerImpl(VisualSystem* visualSystem)
    : mVisualSystem(visualSystem)
{
    mRenderManager = mVisualSystem->getRenderManager();
    mHeightmapGenerator =
        std::make_unique<HeightMapGenerator>(mVisualSystem->getDevice());
    mMaterialStore = std::make_unique<TerrainMaterialStore>(
        mVisualSystem->getDevice(), mVisualSystem->getResourceManager());

    mMaterialStore->initializeMaterials();

    // Because our terrain is heightmap based, we can use a single mesh and
    // instance draw it for each chunk, reading from heightmap texture for the
    // height.
    constexpr int kMeshSampleCount = 15;
    constexpr float kMeshSkirtDepth = 25.f;
    setupTerrainMesh(kMeshSampleCount, kMeshSkirtDepth);
    setupTerrainMaterial();

    invalidateHeightmap();

    reset();

    ImGuiHelper::RegisterImGuiCallback("Render/Terrain2D",
                                       [this]() { imGui(); });
}
Terrain2DManagerImpl::~Terrain2DManagerImpl() = default;

void Terrain2DManagerImpl::setupTerrainMesh(int numSamples, float skirtDepth)
{
    MeshBuilder builder;
    generateTerrainPlaneMesh(builder, numSamples, skirtDepth);
    mTerrainMesh = mVisualSystem->getResourceManager()->requestMesh(builder);
}

void Terrain2DManagerImpl::setupTerrainMaterial()
{
    mTerrainMaterial = std::make_shared<Material>();

    Technique* technique = mTerrainMaterial->setTechnique(RenderPass::kOpaque);
    technique->vertexShader = "Terrain";
    technique->pixelShader = "Terrain";

    ShaderResource colormap{};
    colormap.initializeTextureResource(mMaterialStore->getAlbedoArray(),
                                       SamplerSettings::Linear);
    technique->bindPixelResource(4, colormap);
}

void Terrain2DManagerImpl::updatePerform(const Vector3& cameraPosition,
                                         DeviceContext* context)
{
    PROFILE_SCOPE("Terrain::updatePerform");

    // Stream in terrain materials that are needed
    mMaterialStore->streamMaterials(context);

    if (config.invalidateHeightmap)
    {
        regenerateHeightmapTexture(context);
        config.invalidateHeightmap = false;
    }

    updateIdealQuadTreeLOD(root, cameraPosition, 0);
    selectReadyQuadTreeNodes();
    submitTerrainForRendering();
}

void Terrain2DManagerImpl::imGui()
{
#if defined(IMGUI_ENABLED)
    ImGui::Text("# Chunks: %zu", mQuadTreeAllocator.getNumAllocations());
    ImGui::Text("Pool Pages: %u", mQuadTreeAllocator.getPageCount());
    ImGui::Text("# Leaves: %i", chunksToRender.size());

    ImGui::SliderFloat("LOD Attenuation", &config.lodAttenuation, 0.0, 10000.f);

    ImGui::SliderFloat("Triplanar Texture Scaling: ",
                       &shaderSettings.triplanarTextureScale, 0.0001f, 0.1f);
    ImGui::SliderFloat("Triplanar Texture Sharpness: ",
                       &shaderSettings.triplanarTextureSharpness, 0.01f, 10.f);
    ImGui::SliderFloat(
        "Triplanar Noise Scaling: ", &shaderSettings.noiseScaling, 0.01f, 1.f);

    if (ImGui::CollapsingHeader("Terrain Mesh"))
    {
        static int meshSampleCount = 15;
        static float meshSkirtDepth = 25.f;
        ImGui::SliderInt("# Terrain Mesh Samples: %i", &meshSampleCount, 2, 25);
        ImGui::SliderFloat("Terrain Skirt Depth:", &meshSkirtDepth, 0.f, 50.f);

        if (ImGui::Button("Reset Terrain Mesh"))
        {
            setupTerrainMesh(meshSampleCount, meshSkirtDepth);
        }
    }

    mHeightmapGenerator->imGui();

    ImGui::SliderFloat2("Heightmap Origin:", &config.heightMapOrigin.x, -500,
                        500);
    ImGui::SliderFloat2("Heightmap Extents:", &config.heightMapExtents.x, 10,
                        5000);
    if (ImGui::Button("Reset Heightmap"))
    {
        invalidateHeightmap();
    }

    if (ImGui::CollapsingHeader("Noise Settings"))
    {

        if (ImGui::Button("Reset"))
        {
            reset();
        }
    }
#endif
}

void Terrain2DManagerImpl::reset()
{
    if (root)
    {
        destroyNode(root);
        root = nullptr;
    }

    const float rootSize = kTerrainNodeSize * (1 << kMaxQuadTreeDepth);
    root = allocateNode(Vector2(-rootSize / 2, -rootSize / 2),
                        Vector2(rootSize, rootSize));
}

void Terrain2DManagerImpl::invalidateHeightmap()
{
    config.invalidateHeightmap = true;
}

void Terrain2DManagerImpl::regenerateHeightmapTexture(DeviceContext* context)
{
    const Vector2 xzMin =
        config.heightMapOrigin - config.heightMapExtents / 2.f;
    const Vector2 xzMax =
        config.heightMapOrigin + config.heightMapExtents / 2.f;
    mHeightmapGenerator->generateHeightMap(xzMin, xzMax, context);

    ShaderResource resource;
    resource.initializeTextureResource(mHeightmapGenerator->getTexture(),
                                       SamplerSettings::Linear);
    mTerrainTechnique = mTerrainMaterial->getTechnique(RenderPass::kOpaque);
    mTerrainTechnique->bindVertexResource(0, resource);
}

uint8_t Terrain2DManagerImpl::computeIdealLOD(QuadTreeNode* node,
                                              const Vector3& cameraPosition)
{
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

void Terrain2DManagerImpl::updateIdealQuadTreeLOD(QuadTreeNode* node,
                                                  const Vector3& cameraPosition,
                                                  int depth)
{
    const uint8_t idealLOD = computeIdealLOD(node, cameraPosition);
    if (node->isLeaf())
    {
        if (idealLOD > depth)
        {
            divideNode(*node);

            for (int i = 0; i < 4; i++)
            {
                updateIdealQuadTreeLOD(node->children[i], cameraPosition,
                                       depth + 1);
            }
        }
    }
    else
    {
        if (idealLOD < depth)
        {
            mergeNode(*node);
        }
        else
        {
            for (int i = 0; i < 4; i++)
            {
                updateIdealQuadTreeLOD(node->children[i], cameraPosition,
                                       depth + 1);
            }
        }
    }
}

void Terrain2DManagerImpl::selectReadyQuadTreeNodes()
{
    chunksToRender.clear();

    if (nodeReadyCheck(root))
    {
        selectReadyQuadTreeNodesHelper(root);
    }
}

bool Terrain2DManagerImpl::nodeReadyCheck(QuadTreeNode* node)
{
    return node != nullptr;
}

void Terrain2DManagerImpl::selectReadyQuadTreeNodesHelper(QuadTreeNode* node)
{
    bool submitForDrawing = false;
    assert(nodeReadyCheck(node));

    if (node->isLeaf())
    {
        submitForDrawing = true;
    }
    else
    {
        bool childrenReady = true;
        for (int i = 0; i < 4; i++)
        {
            childrenReady = childrenReady && nodeReadyCheck(node->children[i]);
        }
        submitForDrawing = !childrenReady;
    }

    DrawBlockKey& drawBlockKey = node->blockKey;
    if (submitForDrawing)
    {
        if (drawBlockKey == kInvalidDrawBlockKey)
        {
            DrawBlock drawBlock;
            drawBlock.initialize(mTerrainMesh, mTerrainMaterial);
            drawBlockKey =
                mVisualSystem->getRenderManager()->addDrawBlock(drawBlock);

            Vector3 pos =
                Vector3(node->data.position.x, 50.f, node->data.position.y);
            mVisualSystem->getVisualDebug()->drawPoint(pos, 10.f);
        }

        chunksToRender.push_back(node->data);
    }
    else
    {
        if (drawBlockKey != kInvalidDrawBlockKey)
        {
            mVisualSystem->getRenderManager()->removeDrawBlock(drawBlockKey);
            drawBlockKey = kInvalidDrawBlockKey;
        }

        for (int i = 0; i < 4; i++)
            selectReadyQuadTreeNodesHelper(node->children[i]);
    }
}

void Terrain2DManagerImpl::submitTerrainForRendering()
{
    if (!mTerrainTechnique)
        return;

    mTerrainTechnique->clearPixelCB(4);
    mTerrainTechnique->uploadPixelCBData(4, &shaderSettings,
                                         sizeof(ShaderSettings));

    const bool render = !chunksToRender.empty() && mTerrainMesh->ready;
    if (render)
    {
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
    }
}

QuadTreeNode* Terrain2DManagerImpl::allocateNode(const Vector2& position,
                                                 const Vector2& extents)
{
    QuadTreeNode* node = mQuadTreeAllocator.allocate();
    node->data.position = position;
    node->data.extents = extents;

    return node;
}
void Terrain2DManagerImpl::destroyNode(QuadTreeNode* node)
{
    if (!node->isLeaf())
    {
        for (int i = 0; i < 4; i++)
        {
            destroyNode(node->children[i]);
        }
    }
    mQuadTreeAllocator.free(node);
}

void Terrain2DManagerImpl::divideNode(QuadTreeNode& node)
{
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

void Terrain2DManagerImpl::mergeNode(QuadTreeNode& node)
{
    assert(!node.isLeaf());

    for (int i = 0; i < 4; i++)
    {
        QuadTreeNode* child = node.children[i];
        destroyNode(child);
        node.children[i] = nullptr;
    }
}

} // namespace Graphics
} // namespace Engine