#include "RenderManager.h"

#include "rendering/Direct3D11.h"
#include <d3d11_1.h>

#include "rendering/util/GPUTimer.h"

#include "rendering/VisualSystem.h"

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

struct RenderPassContainer {
    std::vector<DrawCall> drawCalls;
    ID3DUserDefinedAnnotation* debugAnnotation;
};

class RenderManagerImpl {
    ID3D11DeviceContext* context;
    ID3D11Device* device;
    VisualSystem* visualSystem;

    std::array<RenderPassContainer, RenderPass::_Count_> mRenderPasses;

  public:
    RenderManagerImpl(VisualSystem* _visualSystem,
                      ID3D11DeviceContext* _context, ID3D11Device* _device);
    ~RenderManagerImpl();

    void submitDrawCall(const RenderPass pass, const DrawCall& drawCall);
    void perform();

  private:
    void executeRenderPass(const RenderPassContainer& drawCalls,
                           RenderPass pass, const std::string& annotation);
};

RenderManager::RenderManager() = default;
RenderManager::~RenderManager() = default;

void RenderManager::submitDrawCall(const DrawCall& drawCall,
                                   RenderPassSet renderPasses) {
    for (int i = 0; i < RenderPass::_Count_; i++) {
        if (renderPasses.hasPass((RenderPass)i)) {
            mImpl->submitDrawCall((RenderPass)i, drawCall);
        }
    }
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
        context->QueryInterface(
            IID_PPV_ARGS(&mRenderPasses[pass].debugAnnotation));
    }
}
RenderManagerImpl::~RenderManagerImpl() = default;

void RenderManagerImpl::submitDrawCall(const RenderPass pass,
                                       const DrawCall& drawCall) {
    mRenderPasses[pass].drawCalls.emplace_back(drawCall);
}

void RenderManagerImpl::perform() {
    Pipeline* pipeline = visualSystem->getPipeline();

    Camera* camera = visualSystem->getSceneManager()->getMainCamera();
    Matrix4 screenFromWorld =
        camera->getFrustumMatrix() * camera->getWorldToCameraMatrix();

    // Bind my atlases
    visualSystem->getResourceManager()
        ->getTexture(SystemTexture_FallbackColormap)
        ->PSBindResource(context, 0);
    visualSystem->getLightManager()->getAtlasTexture()->PSBindResource(context,
                                                                       1);
    pipeline->getDepthStencil()->clearAsDepthStencil(context);

    // Vertex Constant Buffer 0:
    // Stores the camera view and projection matrices
    {
        IConstantBuffer vCB0 = pipeline->loadVertexCB(CB0);
        vCB0.loadData(&screenFromWorld, FLOAT4X4);
    }

    // Vertex Constant Buffer 1:
    // Stores the camera view and projection matrices
    {
        IConstantBuffer vCB1 = pipeline->loadVertexCB(CB1);

        Camera* camera = visualSystem->getSceneManager()->getMainCamera();
        const Matrix4 viewMatrix = camera->getWorldToCameraMatrix();
        vCB1.loadData(&viewMatrix, FLOAT4X4);
        const Matrix4 projectionMatrix = camera->getFrustumMatrix();
        vCB1.loadData(&projectionMatrix, FLOAT4X4);
    }

    // Pixel Constant Buffer 1: Light Data
    // Stores data that is needed for lighting / shadowing.
    {
        IConstantBuffer pCB1 = pipeline->loadPixelCB(CB1);
        visualSystem->getLightManager()->bindLightData(pCB1);
    }

    {
        pipeline->bindRenderTarget(Target_UseExisting, Depth_TestAndWrite,
                                   Blend_Default);
        executeRenderPass(mRenderPasses[RenderPass::kOpaque],
                          RenderPass::kOpaque, "Opaque");
    }

    // Clear all draw calls for the next frame.
    for (int pass = 0; pass < RenderPass::_Count_; pass++) {
        mRenderPasses[pass].drawCalls.clear();
    }
}

void RenderManagerImpl::executeRenderPass(const RenderPassContainer& drawCalls,
                                          RenderPass pass,
                                          const std::string& annotation) {
    Pipeline* pipeline = visualSystem->getPipeline();

    {
        DebugRenderPassScope renderpass_debug =
            DebugRenderPassScope(drawCalls.debugAnnotation, annotation);
        IGPUTimer gpu_timer = GPUTimer::TrackGPUTime(annotation);

        for (auto& drawCall : drawCalls.drawCalls) {
            drawCall.pixelTechnique->bind(pipeline);
            drawCall.vertexTechnique->bindAndDraw(pipeline, pass);
        }
    }
}

} // namespace Graphics
} // namespace Engine