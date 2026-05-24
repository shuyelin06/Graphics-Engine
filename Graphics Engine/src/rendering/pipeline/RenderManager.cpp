#include "RenderManager.h"

#include "core/PoolAllocator.h"

#include "d3d11.h"
#include "rendering/Direct3D11.h"
#include <d3d11_1.h>

#include "rendering/util/GPUTimer.h"

#include "DrawCall.h"
#include "rendering/VisualSystem.h"

#include <algorithm>
#include <assert.h>
#include <vector>

namespace Engine {
namespace Graphics {
// DebugRenderPassScope:
// Responsible for RenderDoc annotations when executing a Render Pass.
// Created at the beginning of the pass, and automatically ends the
// event on destruction
class DebugRenderPassScope {
    ID3DUserDefinedAnnotation* annotation;

  public:
    DebugRenderPassScope(ID3DUserDefinedAnnotation* target_annotation,
                         const std::string& name) {
        annotation = target_annotation;

        const std::wstring wstring = std::wstring(name.begin(), name.end());
        annotation->BeginEvent(wstring.c_str());
    }
    ~DebugRenderPassScope() { annotation->EndEvent(); }
};

DrawBlock::DrawBlock() = default;

void DrawBlock::initialize(AABB _extents, Mesh* _mesh, Material* _material) {
    extents = _extents;
    mesh = _mesh;
    material = _material;
}

struct GlobalPixelShaderData {
    Vector3 viewPosition = Vector3();
    float viewZNear = 0.f;

    Vector3 viewDirection = Vector3(1, 0, 0);
    float viewZFar = 0.f;

    Matrix4 mWorldToScreen = Matrix4::Identity();
    Matrix4 mScreenToWorld = Matrix4::Identity();

    Vector4 resolutionInfo = Vector4();
};

class RenderManagerImpl {
    ID3D11DeviceContext* context;
    ID3D11Device* device;
    VisualSystem* visualSystem;

    std::array<ID3DUserDefinedAnnotation*, RenderPass::_Count_>
        mDebugAnnotations;

    // TODO: Octree + Culling
    DrawBlockKey counter = 0;
    std::unordered_map<DrawBlockKey, DrawBlock> drawBlocks;

    // Instance Data
    PoolAllocator<InstanceData, 4096 * 16 / sizeof(InstanceData)>
        instanceDataPool;
    bool instanceDataDirty;

    // Constant Buffer Data
    RenderView mainView;

    GlobalPixelShaderData pcb0Data;

  public:
    RenderManagerImpl(VisualSystem* _visualSystem,
                      ID3D11DeviceContext* _context, ID3D11Device* _device);
    ~RenderManagerImpl();

    // TODO: This should be thread safe.
    DrawBlockKey addDrawBlock(const DrawBlock& block);
    void updateInstanceData(const DrawBlockKey key, InstanceData instanceData);
    void removeDrawBlock(const DrawBlockKey);

    void setMainView(const RenderView& view);
    void setShadowViews(const RenderView* viewArr, uint32_t count);

    void perform();

  private:
    void executeRenderPass(RenderPass pass, const std::string& annotation);
};

RenderManager::RenderManager() = default;
RenderManager::~RenderManager() = default;

DrawBlockKey RenderManager::addDrawBlock(const DrawBlock& block) {
    return mImpl->addDrawBlock(block);
}

void RenderManager::updateInstanceData(const DrawBlockKey key,
                                       InstanceData instanceData) {
    return mImpl->updateInstanceData(key, instanceData);
}

void RenderManager::removeDrawBlock(const DrawBlockKey key) {
    mImpl->removeDrawBlock(key);
}

void RenderManager::setMainView(const RenderView& view) {
    mImpl->setMainView(view);
}

void RenderManager::perform() { mImpl->perform(); }

std::unique_ptr<RenderManager>
RenderManager::create(VisualSystem* visual_system, ID3D11DeviceContext* context,
                      ID3D11Device* device) {
    std::unique_ptr<RenderManager> ptr =
        std::unique_ptr<RenderManager>(new RenderManager());
    ptr->mImpl =
        std::make_unique<RenderManagerImpl>(visual_system, context, device);
    return ptr;
}

RenderManagerImpl::RenderManagerImpl(VisualSystem* _visualSystem,
                                     ID3D11DeviceContext* _context,
                                     ID3D11Device* _device)
    : visualSystem(_visualSystem), context(_context), device(_device) {
    for (int pass = 0; pass < RenderPass::_Count_; pass++) {
        context->QueryInterface(IID_PPV_ARGS(&mDebugAnnotations[pass]));
    }

    instanceDataDirty = false;

    counter = 0;
}
RenderManagerImpl::~RenderManagerImpl() = default;

DrawBlockKey RenderManagerImpl::addDrawBlock(const DrawBlock& block) {
    const DrawBlockKey key = counter++;
    drawBlocks[key] = block;
    return key;
}

void RenderManagerImpl::updateInstanceData(const DrawBlockKey key,
                                           InstanceData instanceData) {
    assert(drawBlocks.contains(key));
    auto& drawBlock = drawBlocks[key];

    instanceDataDirty = true;

    if (drawBlock.instanceData) {
        instanceDataPool.free(drawBlock.instanceData);
    }

    drawBlock.instanceData = instanceDataPool.allocate();
    *drawBlock.instanceData = instanceData;
}

void RenderManagerImpl::removeDrawBlock(const DrawBlockKey key) {
    assert(drawBlocks.contains(key));

    auto& drawBlock = drawBlocks[key];
    if (drawBlock.instanceData) {
        instanceDataPool.free(drawBlock.instanceData);
    }

    drawBlocks.erase(key);
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
void RenderManagerImpl::perform() {
    Pipeline* pipeline = visualSystem->getPipeline();
    Texture* renderTarget = pipeline->getRenderTargetDest();
    Texture* depthStencil = pipeline->getDepthStencil();

    // Bind my atlases
    visualSystem->getResourceManager()
        ->getTexture(SystemTexture_FallbackColormap)
        ->PSBindResource(context, 0);
    visualSystem->getLightManager()->getAtlasTexture()->PSBindResource(context,
                                                                       1);
    pipeline->getDepthStencil()->clearAsDepthStencil(context);

    // Bind my global constant buffers (CB0)
    // Vertex Constant Buffer 0:
    // Stores the camera view and projection matrices
    pipeline->markVertexCBUsage(0, true);
    pipeline->markVertexCBUsage(3, true);
    pipeline->markPixelCBUsage(0, true);

    {
        IConstantBuffer vCB0 = pipeline->loadVertexCB(0);
        Matrix4 screenFromWorld =
            mainView.mLocalToFrustum * mainView.mWorldToLocal;
        vCB0.loadData(&screenFromWorld, FLOAT4X4);
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

        IConstantBuffer pcb0_common = pipeline->loadPixelCB(0);
        pcb0_common.loadData(&pcb0Data, sizeof(pcb0Data));
    }

    // Vertex Constant Buffer 1:
    // Stores the camera view and projection matrices
    {
        IConstantBuffer vCB1 = pipeline->loadVertexCB(1);

        const Matrix4 viewMatrix = mainView.mWorldToLocal;
        vCB1.loadData(&viewMatrix, FLOAT4X4);
        const Matrix4 projectionMatrix = mainView.mLocalToFrustum;
        vCB1.loadData(&projectionMatrix, FLOAT4X4);
    }

    // Pixel Constant Buffer 1: Light Data
    // Stores data that is needed for lighting / shadowing.
    {
        IConstantBuffer pCB1 = pipeline->loadPixelCB(1);
        visualSystem->getLightManager()->bindLightData(pCB1);
    }

    // Vertex Constant Buffer 3: Instance Data
    {
        // TODO We only need to reupload if instance data was uploaded this
        // frame. But that can only be done once we are completely migrated to
        // RenderManager.

        // if (instanceDataDirty)
        {
            IConstantBuffer vCB3 = pipeline->loadVertexCB(3);
            vCB3.loadData(instanceDataPool.getData(),
                          instanceDataPool.getSize() * sizeof(InstanceData));
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
        pipeline->bindRenderTarget(mainView.renderTarget, mainView.depthStencil,
                                   Depth_TestAndWrite);
        pipeline->bindBlendSettings(Blend_Default);
        executeRenderPass(RenderPass::kOpaque, "Opaque");
    }

    {
        pipeline->bindRenderTarget(mainView.renderTarget, mainView.depthStencil,
                                   Depth_TestAndWrite);
        pipeline->bindBlendSettings(Blend_Default);
        executeRenderPass(RenderPass::kDebug, "Debug");
    }

    pipeline->markVertexCBUsage(0, false);
    pipeline->markVertexCBUsage(3, false);
    pipeline->markPixelCBUsage(0, false);
}

void RenderManagerImpl::executeRenderPass(RenderPass pass,
                                          const std::string& annotation) {
    Pipeline* pipeline = visualSystem->getPipeline();

    // Query my draw blocks
    // TODO This is quite inefficient. We should move this to a job or something
    // later.
    std::vector<DrawCall> drawCallsEx;
    for (const auto& pair : drawBlocks) {
        const DrawBlock& drawBlock = pair.second;
        const Technique* technique = drawBlock.material->getTechnique(pass);

        if (technique != nullptr) {
            DrawCall call;
            call.mesh = drawBlock.mesh;
            call.technique = technique;
            if (drawBlock.instanceData) {
                call.instanceDataIndex =
                    instanceDataPool.getIndex(drawBlock.instanceData);
            }
            drawCallsEx.push_back(call);
        }
    }
    std::sort(drawCallsEx.begin(), drawCallsEx.end(),
              [](const DrawCall& a, const DrawCall& b) {
                  return (a.depth <= b.depth) && (a.technique <= b.technique) &&
                         (a.mesh <= b.mesh);
              });

    DebugRenderPassScope renderpass_debug =
        DebugRenderPassScope(mDebugAnnotations[pass], annotation);
    IGPUTimer gpu_timer = GPUTimer::TrackGPUTime(annotation);

    // TODO These just call pipeline methods. Move it to pipeline
    // (i.e. pipeline->draw(VertexTechnique, PixelTechnique).
    // TODO ResourceManager needs to enforce lifetime of the pointer
    // resources
    size_t head = 0;
    size_t tail = 0;

    bool stop = false;
    while (tail < drawCallsEx.size()) {
        bool draw = true;

        const DrawCall& drawCall = drawCallsEx[tail];

        // Check if we can batch.
        // We can only batch if everything is equal except for the instance
        // data handle (and the instance data handle is provided)
        bool canBatch = drawCall.instanceDataIndex != kInvalidInstanceDataKey;
        {
            const DrawCall& drawCallInBatch = drawCallsEx[head];
            (drawCallInBatch.depth == drawCall.depth);
            canBatch =
                canBatch && (drawCallInBatch.technique == drawCall.technique);
            canBatch = canBatch && (drawCallInBatch.mesh == drawCall.mesh);
        }
        // Do not batch if we are on the last draw call of the list. Just render
        // what we have.
        canBatch = canBatch && (tail != drawCallsEx.size() - 1);

        draw = !canBatch;

        if (draw) {
            // Bind everything
            // TODO Shader Resources

            const Mesh* mesh = drawCall.mesh;
            const Technique* technique = drawCall.technique;

            pipeline->bindVertexShader(technique->vertexShader);
            for (int slot = 0; slot < kVertexConstantBufferMax; slot++) {
                const auto& buffer = technique->vertexCBuffers[slot];
                if (buffer.size() > 0) {
                    pipeline->markVertexCBUsage(slot, true);

                    IConstantBuffer cbHandle = pipeline->loadVertexCB(slot);
                    cbHandle.loadData(buffer.data(), buffer.size());

                    pipeline->markVertexCBUsage(slot, false);
                }
            }

            pipeline->bindPixelShader(technique->pixelShader);
            for (int slot = 0; slot < kVertexConstantBufferMax; slot++) {
                const auto& buffer = technique->pixelCbuffers[slot];
                if (buffer.size() > 0) {
                    pipeline->markPixelCBUsage(slot, true);

                    IConstantBuffer cbHandle = pipeline->loadPixelCB(slot);
                    cbHandle.loadData(buffer.data(), buffer.size());

                    pipeline->markPixelCBUsage(slot, false);
                }
            }

            // 2 Paths:
            // - Instanced Draw Call. We upload the instance handles into Cb4.
            // - Non-Instanced Draw Call. We still do the instanced draw call
            // API, but do not touch CB4.
            int numInstances = (tail - head) + 1;
            if (drawCall.instanceDataIndex != kInvalidInstanceDataKey) {
                IConstantBuffer cbHandle = pipeline->loadVertexCB(4);

                for (; head <= tail; head++) {
                    assert(sizeof(InstanceDataKey) == sizeof(uint32_t));
                    cbHandle.loadData(&(drawCallsEx[head].instanceDataIndex),
                                      sizeof(InstanceDataKey));
                }
                // cbHandle.loadData(nullptr, 16);
            }

            pipeline->drawMesh(mesh, numInstances);
        }

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