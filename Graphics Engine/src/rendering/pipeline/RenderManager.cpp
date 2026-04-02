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

PipelineRenderPassSet::PipelineRenderPassSet(uint8_t _bitMask) {
    bitMask = _bitMask;
}
bool PipelineRenderPassSet::hasRenderPass(PipelineRenderPass pass) const {
    return (bitMask & (1 << pass)) == (1 << pass);
}

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

    void submitDrawCall(const PipelineRenderPassSet passes,
                        const DrawCall& drawCall);
    void perform();
};

RenderManager::RenderManager() = default;
RenderManager::~RenderManager() = default;

void RenderManager::submitDrawCall(const PipelineRenderPassSet passes,
                                   const DrawCall& drawCall) {
    mImpl->submitDrawCall(passes, drawCall);
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

void RenderManagerImpl::submitDrawCall(const PipelineRenderPassSet passes,
                                       const DrawCall& drawCall) {
    for (int pass = 0; pass < PipelineRenderPass::kRenderPass_Count_; pass++) {
        if (passes.hasRenderPass((PipelineRenderPass)pass)) {
            mRenderPasses[pass].drawCalls.push_back(drawCall);
        }
    }
}

void RenderManagerImpl::perform() {
#define RENDER_PASS(pin, name)                                                 \
    DebugRenderPassScope renderpass_debug =                                    \
        DebugRenderPassScope(mRenderPasses[pin].debugAnnotation, name);        \
    IGPUTimer gpu_timer = GPUTimer::TrackGPUTime(name);

    Pipeline* pipeline = visualSystem->getPipeline();
    LightManager* lightManager = visualSystem->getLightManager();

    // Pixel Constant Buffer 1: Light Data
    // Stores data that is needed for lighting / shadowing.
    {
        IConstantBuffer pCB1 = pipeline->loadPixelCB(CB1);
        lightManager->bindLightData(pCB1);
    }

    // For now, only support terrain
    assert(PipelineRenderPass::kRenderPass_Count_ == 1);

    {
        RENDER_PASS(PipelineRenderPass::kRenderPass_Terrain, "Terrain");
        for (auto& drawCall :
             mRenderPasses[PipelineRenderPass::kRenderPass_Terrain].drawCalls) {
            if (drawCall.material)
                drawCall.material->bind(pipeline, context);
            drawCall.mesh->bindAndExecute(pipeline, context);
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