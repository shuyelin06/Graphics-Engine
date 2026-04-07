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

    std::array<RenderPassContainer, PipelineRenderPass::kRenderPass_Count_>
        mRenderPasses;

  public:
    RenderManagerImpl(VisualSystem* _visualSystem,
                      ID3D11DeviceContext* _context, ID3D11Device* _device);
    ~RenderManagerImpl();

    void submitDrawCall(const PipelineRenderPass pass,
                        const DrawCall& drawCall);
    void perform();
};

RenderManager::RenderManager() = default;
RenderManager::~RenderManager() = default;

void RenderManager::submitDrawCall_Opaque(VertexTechnique* vertexTechnique,
                                          PixelTechnique* pixelTechnique) {
    mImpl->submitDrawCall(PipelineRenderPass::kRenderPass_Opaque,
                          {vertexTechnique, pixelTechnique});
}
void RenderManager::submitDrawCall_Terrain(VertexTechnique* vertexTechnique,
                                           PixelTechnique* pixelTechnique) {
    mImpl->submitDrawCall(PipelineRenderPass::kRenderPass_Terrain,
                          {vertexTechnique, pixelTechnique});
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
    for (int pass = 0; pass < PipelineRenderPass::kRenderPass_Count_; pass++) {
        context->QueryInterface(
            IID_PPV_ARGS(&mRenderPasses[pass].debugAnnotation));
    }
}
RenderManagerImpl::~RenderManagerImpl() = default;

void RenderManagerImpl::submitDrawCall(const PipelineRenderPass pass,
                                       const DrawCall& drawCall) {
    mRenderPasses[pass].drawCalls.emplace_back(drawCall);
}

void RenderManagerImpl::perform() {
#define RENDER_PASS(pin, name)                                                 \
    DebugRenderPassScope renderpass_debug =                                    \
        DebugRenderPassScope(mRenderPasses[pin].debugAnnotation, name);        \
    IGPUTimer gpu_timer = GPUTimer::TrackGPUTime(name);

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

    // Vertex Constant Buffer 0:
    // Stores the camera view and projection matrices
    {
        IConstantBuffer vCB0 = pipeline->loadVertexCB(CB0);
        vCB0.loadData(&screenFromWorld, FLOAT4X4);
    }

    {
        RENDER_PASS(PipelineRenderPass::kRenderPass_Terrain, "Terrain");
        for (auto& drawCall :
             mRenderPasses[PipelineRenderPass::kRenderPass_Terrain].drawCalls) {
            drawCall.pixelTechnique->bind(pipeline);
            drawCall.vertexTechnique->bindAndDraw(
                pipeline, PipelineRenderPass::kRenderPass_Terrain);
        }
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

    {
        RENDER_PASS(PipelineRenderPass::kRenderPass_Opaque, "Opaque");
        for (auto& drawCall :
             mRenderPasses[PipelineRenderPass::kRenderPass_Opaque].drawCalls) {
            drawCall.pixelTechnique->bind(pipeline);
            drawCall.vertexTechnique->bindAndDraw(
                pipeline, PipelineRenderPass::kRenderPass_Opaque);
        }
    }

    // Clear all draw calls for the next frame.
    for (int pass = 0; pass < PipelineRenderPass::kRenderPass_Count_; pass++) {
        mRenderPasses[pass].drawCalls.clear();
    }

#undef RENDER_PASS
}

} // namespace Graphics
} // namespace Engine