#include "RenderManager.h"

#include "core/PoolAllocator.h"
#include "core/Slotmap.h"

#include "rendering/core/Frustum.h"

#include "DrawCall.h"
#include "rendering/VisualSystem.h"

#include <algorithm>
#include <assert.h>
#include <vector>

namespace Engine
{
namespace Graphics
{
DrawBlock::DrawBlock() = default;

void DrawBlock::initialize(std::shared_ptr<Geometry>& _mesh,
                           std::shared_ptr<Material>& _material)
{
    mesh = _mesh;
    material = _material;
}

struct DrawBlockImpl
{
    std::shared_ptr<Geometry> mesh = nullptr;
    std::shared_ptr<Material> material = nullptr;

    AABB extents{};

    // If nullptr, draws with the identity instance handle (0)
    InstanceData* instanceData = nullptr;

    DrawBlockImpl() = default;
    DrawBlockImpl(const DrawBlock& drawBlock)
        : mesh(drawBlock.mesh)
        , material(drawBlock.material)
    {
    }
};

struct GlobalPixelShaderData
{
    Vector3 viewPosition = Vector3();
    float viewZNear = 0.f;

    Vector3 viewDirection = Vector3(1, 0, 0);
    float viewZFar = 0.f;

    Matrix4 mWorldToScreen = Matrix4::Identity();
    Matrix4 mScreenToWorld = Matrix4::Identity();

    Vector4 resolutionInfo = Vector4();
};

class RenderManagerImpl
{
    ID3D11DeviceContext* context;
    ID3D11Device* device;
    VisualSystem* visualSystem;

    // TODO: Octree + Culling
    SlotMap<DrawBlockImpl> drawBlocks;

    // Instance Data
    PoolAllocator<InstanceData,
                  4096 * 16 / sizeof(InstanceData),
                  PoolAllocatorPolicy::FixedSize>
        instanceDataPool;

    // Constant Buffer Data
    RenderView mainView;

    GlobalPixelShaderData pcb0Data;

  public:
    RenderManagerImpl(VisualSystem* _visualSystem,
                      ID3D11DeviceContext* _context,
                      ID3D11Device* _device);
    ~RenderManagerImpl();

    // TODO: This should be thread safe.
    DrawBlockKey addDrawBlock(const DrawBlock& block);
    void updateInstanceData(const DrawBlockKey key, InstanceData instanceData);
    void removeDrawBlock(const DrawBlockKey);

    void setMainView(const RenderView& view);
    void setShadowViews(const RenderView* viewArr, uint32_t count);

    void perform();

  private:
    void executeRenderPass(DeviceContext* context,
                           RenderPass pass,
                           const RenderView& view,
                           const std::string& annotation);
    void buildVisibleSet(const RenderView& view,
                         std::vector<DrawBlockKey>& visibleBlocks);
    void buildRenderPass(RenderPass pass,
                         const std::vector<DrawBlockKey>& visibleBlocks,
                         std::vector<DrawCall>& drawCalls);
    void renderDrawCalls(DeviceContext* context,
                         const std::vector<DrawCall>& drawCalls);
};

RenderManager::RenderManager() = default;
RenderManager::~RenderManager() = default;

DrawBlockKey RenderManager::addDrawBlock(const DrawBlock& block)
{
    return mImpl->addDrawBlock(block);
}

void RenderManager::updateInstanceData(const DrawBlockKey key,
                                       InstanceData instanceData)
{
    return mImpl->updateInstanceData(key, instanceData);
}

void RenderManager::removeDrawBlock(const DrawBlockKey key)
{
    mImpl->removeDrawBlock(key);
}

void RenderManager::setMainView(const RenderView& view)
{
    mImpl->setMainView(view);
}

void RenderManager::perform() { mImpl->perform(); }

std::unique_ptr<RenderManager>
RenderManager::create(VisualSystem* visual_system,
                      ID3D11DeviceContext* context,
                      ID3D11Device* device)
{
    std::unique_ptr<RenderManager> ptr =
        std::unique_ptr<RenderManager>(new RenderManager());
    ptr->mImpl =
        std::make_unique<RenderManagerImpl>(visual_system, context, device);
    return ptr;
}

RenderManagerImpl::RenderManagerImpl(VisualSystem* _visualSystem,
                                     ID3D11DeviceContext* _context,
                                     ID3D11Device* _device)
    : visualSystem(_visualSystem)
    , context(_context)
    , device(_device)
{
    // Allocate index 0 of the InstanceData pool for the identity Instance
    InstanceData* identity = instanceDataPool.allocate();
    assert(instanceDataPool.getIndex(identity) == kIdentityInstanceDataKey);
    *identity = InstanceData();
}
RenderManagerImpl::~RenderManagerImpl() = default;

DrawBlockKey RenderManagerImpl::addDrawBlock(const DrawBlock& block)
{
    const DrawBlockKey key = drawBlocks.allocate();
    drawBlocks.get(key) = DrawBlockImpl(block);
    return key;
}

void RenderManagerImpl::updateInstanceData(const DrawBlockKey key,
                                           InstanceData instanceData)
{
    assert(drawBlocks.contains(key));
    auto& drawBlock = drawBlocks.get(key);

    if (drawBlock.instanceData)
    {
        instanceDataPool.free(drawBlock.instanceData);
    }

    static const InstanceData kIdentityInstanceData = InstanceData();
    const bool isIdentityInstance =
        memcmp(&instanceData, &kIdentityInstanceData, sizeof(InstanceData)) ==
        0;
    if (!isIdentityInstance)
    {
        drawBlock.instanceData = instanceDataPool.allocate();
        *drawBlock.instanceData = instanceData;
    }
    else
    {
        drawBlock.instanceData == nullptr;
    }
}

void RenderManagerImpl::removeDrawBlock(const DrawBlockKey key)
{
    assert(drawBlocks.contains(key));

    auto& drawBlock = drawBlocks.get(key);
    if (drawBlock.instanceData)
    {
        instanceDataPool.free(drawBlock.instanceData);
    }

    drawBlocks.destroy(key);
}

void RenderManagerImpl::setMainView(const RenderView& view) { mainView = view; }

// Executes the Render Pipeline. Critical Path.
// A couple of assumptions are made here:
// --- CBUFFER LAYOUT ---
// CB0 is the global buffer. It is set once per frame.
// CB1 is the render pass buffer. It is set once per render pass.
// CB2 is the draw call buffer. It is set once per draw call.
// CB3 is the instance buffer. It stores instance data.
// Other constant buffers are unallocated and can be used for whatever.
void RenderManagerImpl::perform()
{
    Pipeline* pipeline = visualSystem->getPipeline();
    ResourceManager* resourceManager = visualSystem->getResourceManager();

    DeviceContext* context = pipeline->getContext();

    const std::shared_ptr<Texture> renderTarget =
        pipeline->getRenderTargetDest();
    const std::shared_ptr<Texture> depthStencil = pipeline->getDepthStencil();

    // Bind my atlases
    const std::shared_ptr<Texture>& colormap =
        resourceManager->getFallbackColormap();
    const std::shared_ptr<Texture>& shadowAtlas =
        visualSystem->getLightManager()->getAtlasTexture();
    context->bindPixelTexture(0, colormap, SamplerSettings::Point);
    context->bindPixelTexture(1, shadowAtlas, SamplerSettings::Shadow);
    context->clearDepthStencil(depthStencil);

    // Bind my global constant buffers (CB0)
    // Vertex Constant Buffer 0:
    // Stores the camera view and projection matrices
    {
        Matrix4 screenFromWorld =
            mainView.mLocalToFrustum * mainView.mWorldToLocal;
        context->loadVertexCB(0, &screenFromWorld, sizeof(screenFromWorld));
    }

    {
        pcb0Data.viewPosition = mainView.position;
        pcb0Data.viewZNear = mainView.zNear;
        pcb0Data.viewDirection = mainView.direction;
        pcb0Data.viewZFar = mainView.zFar;
        pcb0Data.mWorldToScreen =
            mainView.mLocalToFrustum * mainView.mWorldToLocal;
        pcb0Data.mScreenToWorld = pcb0Data.mWorldToScreen.inverse();
        pcb0Data.resolutionInfo = mainView.viewport;
        context->loadPixelCB(0, &pcb0Data, sizeof(pcb0Data));
    }

    // Vertex Constant Buffer 1:
    // Stores the camera view and projection matrices
    {
        struct VCB1
        {
            Matrix4 viewMatrix;
            Matrix4 projectionMatrix;
        };
        VCB1 vcb1Data;
        vcb1Data.viewMatrix = mainView.mWorldToLocal;
        vcb1Data.projectionMatrix = mainView.mLocalToFrustum;
        context->loadVertexCB(1, &vcb1Data, sizeof(vcb1Data));
    }

    // Pixel Constant Buffer 1: Light Data
    // Stores data that is needed for lighting / shadowing.
    {
        visualSystem->getLightManager()->bindLightData(context);
    }

    // Vertex Constant Buffer 3: Instance Data
    {
        // TODO We only need to reupload if instance data was uploaded this
        // frame. But that can only be done once we are completely migrated to
        // RenderManager.

        // if (instanceDataDirty)
        {
            context->loadVertexCB(3, instanceDataPool.getData(),
                                  instanceDataPool.getSize() *
                                      sizeof(InstanceData));
        }
    }

    {
        LightManager* lightManager = visualSystem->getLightManager();
        /*
        pipeline->bindRenderTarget(Target_UseExisting, Depth_TestAndWrite,
                                   Blend_Default);
        executeRenderPass(RenderPass::kOpaque, "Opaque");
        */
    }

    {
        context->bindRenderTarget(mainView.renderTarget, mainView.depthStencil,
                                  DepthSettings::Depth_TestAndWrite,
                                  BlendSettings::Blend_Default);
        executeRenderPass(context, RenderPass::kOpaque, mainView, "Opaque");
    }

    {
        context->bindRenderTarget(mainView.renderTarget, mainView.depthStencil,
                                  DepthSettings::Depth_TestAndWrite,
                                  BlendSettings::Blend_Default);
        executeRenderPass(context, RenderPass::kDebug, mainView, "Debug");
    }
}

void RenderManagerImpl::executeRenderPass(DeviceContext* context,
                                          RenderPass pass,
                                          const RenderView& view,
                                          const std::string& annotation)
{
    std::vector<DrawBlockKey> visibleBlocks;
    std::vector<DrawCall> drawCalls;
    // TODO Visible Set building can be reused for a single view
    buildVisibleSet(view, visibleBlocks);
    buildRenderPass(pass, visibleBlocks, drawCalls);
    renderDrawCalls(context, drawCalls);
}

void RenderManagerImpl::buildVisibleSet(
    const RenderView& view, std::vector<DrawBlockKey>& visibleBlocks)
{
    visibleBlocks.clear();

    const Frustum viewFrustum =
        Frustum(view.mLocalToFrustum * view.mWorldToLocal);

    auto iter = drawBlocks.begin();
    while (iter != drawBlocks.end())
    {
        const DrawBlockKey key = iter.handle();
        const DrawBlockImpl& drawBlock = *iter;

        bool isVisible = true;
        // Frustum Culling Check
        if (drawBlock.extents != AABB() && drawBlock.instanceData)
        {
            const Matrix4 localToWorld = drawBlock.instanceData->mLocalToWorld;
            const OBB obb = OBB(drawBlock.extents, localToWorld);
            isVisible = viewFrustum.intersectsOBB(obb);
        }

        if (isVisible)
        {
            visibleBlocks.push_back(key);
        }

        ++iter;
    }
}

void RenderManagerImpl::buildRenderPass(
    RenderPass pass,
    const std::vector<DrawBlockKey>& visibleBlocks,
    std::vector<DrawCall>& drawCalls)
{
    drawCalls.clear();

    for (const DrawBlockKey& visibleBlockKey : visibleBlocks)
    {
        const DrawBlockImpl& drawBlock = drawBlocks.get(visibleBlockKey);
        const Technique* technique = drawBlock.material->getTechnique(pass);

        if (technique != nullptr)
        {
            DrawCall call;
            call.mesh = drawBlock.mesh.get();
            call.technique = technique;
            if (drawBlock.instanceData)
            {
                call.instanceDataIndex =
                    instanceDataPool.getIndex(drawBlock.instanceData);
            }
            drawCalls.push_back(call);
        }
    }

    std::sort(drawCalls.begin(), drawCalls.end(),
              [](const DrawCall& a, const DrawCall& b) { return a < b; });
}

void RenderManagerImpl::renderDrawCalls(DeviceContext* context,
                                        const std::vector<DrawCall>& drawCalls)
{
    Pipeline* pipeline = visualSystem->getPipeline();

    DrawCall drawCallBatch{};
    std::vector<InstanceDataKey> instanceDataIndices;

    auto batchDrawCall = [&drawCallBatch,
                          &instanceDataIndices](const DrawCall& draw) {
        if (drawCallBatch == draw)
        {
            instanceDataIndices.push_back(draw.instanceDataIndex);
            return true;
        }
        else
        {
            return false;
        }
    };

    size_t tail = 0;

    bool stop = false;
    while (tail < drawCalls.size())
    {
        instanceDataIndices.clear();

        // Begin draw call batch. Take the first draw call in my list
        drawCallBatch = drawCalls[tail];
        batchDrawCall(drawCallBatch);
        ++tail;

        // Attempt to batch subsequent draw calls with this one
        // While batchDrawCall returns true, we are batching.
        while (tail < drawCalls.size() && batchDrawCall(drawCalls[tail]))
        {
            ++tail;
        }

        // Execute draw call batch
        const Geometry* mesh = drawCallBatch.mesh;
        const Technique* technique = drawCallBatch.technique;

        context->bindShaderProgram(technique->vertexShader.c_str(),
                                   technique->pixelShader.c_str());
        for (int slot = 0; slot < kVertexConstantBufferMax; slot++)
        {
            const auto& buffer = technique->vertexCBuffers[slot];
            context->loadVertexCB(slot, buffer.data(), buffer.size());
        }

        for (int slot = 0; slot < kVertexResourceMax; slot++)
        {
            const auto& vertexResource = technique->getVertexResource(slot);
            if (vertexResource.bound)
            {
                context->bindVertexTexture(
                    slot, vertexResource.textureData.texture,
                    vertexResource.textureData.sampleState);
            }
        }

        for (int slot = 0; slot < kVertexConstantBufferMax; slot++)
        {
            const auto& buffer = technique->pixelCbuffers[slot];
            context->loadPixelCB(slot, buffer.data(), buffer.size());
        }

        for (int slot = 0; slot < kPixelResourceMax; slot++)
        {
            const auto& pixelResource = technique->getPixelResource(slot);
            if (pixelResource.bound)
            {
                context->bindPixelTexture(
                    slot, pixelResource.textureData.texture,
                    pixelResource.textureData.sampleState);
            }
        }

        context->loadVertexCB(4, instanceDataIndices.data(),
                              sizeof(InstanceDataKey) *
                                  instanceDataIndices.size());
        context->draw(mesh, instanceDataIndices.size());

        tail++;
    }
}

/*
    // Old skinning code that needs to be ported...
    for (const AssetComponent* comp : asset_components) {
        const Asset* asset = comp->getAsset();

        if (asset->isSkinned()) {
            pipeline->bindVertexShader("SkinnedMesh");
        } else
            pipeline->bindVertexShader("TexturedMesh");

        for (const auto& mesh : asset->getMeshes()) {

            const Material mat = mesh->material;

            // Pixel CB2: Mesh Material Data
            {
                IConstantBuffer pCB2 = pipeline->loadPixelCB(CB2);

                const TextureRegion& region = mat.tex_region;
                pCB2.loadData(&region.x, FLOAT);
                pCB2.loadData(&region.y, FLOAT);
                pCB2.loadData(&region.width, FLOAT);
                pCB2.loadData(&region.height, FLOAT);
            }

            // Vertex CB2: Transform matrices
            const Matrix4& mLocalToWorld = comp->getLocalToWorldMatrix();
            {
                IConstantBuffer vCB2 = pipeline->loadVertexCB(CB2);

                // Load mesh vertex transformation matrix
                vCB2.loadData(&mLocalToWorld, FLOAT4X4);
                // Load mesh normal transformation matrix
                Matrix4 normalTransform =
   mLocalToWorld.inverse().transpose(); vCB2.loadData(&(normalTransform),
   FLOAT4X4);
            }

            // Skinning
            if (asset->isSkinned()) {
                // Vertex CB3: Joint Matrices
                {
                    IConstantBuffer vCB3 = pipeline->loadVertexCB(CB3);

                    const std::vector<SkinJoint>& skin =
   asset->getSkinJoints(); for (int i = 0; i < skin.size(); i++) {
                        // SUPER INEFFICIENT RN
                        // TODO: THIS IS BOTTLE NECKING MY CODE
                        const Matrix4 skin_matrix =
                            skin[i].getTransform(skin[i].node) *
                            skin[i].m_inverse_bind;
                        const Matrix4 skin_normal_matrix =
                            skin_matrix.inverse().transpose();

                        vCB3.loadData(&skin_matrix, FLOAT4X4);
                        vCB3.loadData(&skin_normal_matrix, FLOAT4X4);
                    }
                }
            }

            // Draw each mesh
            pipeline->drawMesh(mesh.get(), INDEX_LIST_START, INDEX_LIST_END,
   1);
        }
    }
*/

} // namespace Graphics
} // namespace Engine