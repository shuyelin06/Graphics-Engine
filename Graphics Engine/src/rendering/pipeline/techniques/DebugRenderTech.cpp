#include "DebugRenderTech.h"

namespace Engine {
namespace Graphics {
VSDebugRenderLine::VSDebugRenderLine() = default;

void VSDebugRenderLine::initialize(ID3D11Device* device) {
    sb_lines.initialize(device, 2000);
}

void VSDebugRenderLine::uploadData(ID3D11DeviceContext* context,
                                   const std::vector<LinePoint>& data) {
    sb_lines.uploadData(context, data.data(), data.size());
    numLines = data.size();
}

// VertexTechnique Implementation
void VSDebugRenderLine::bindAndDraw(Pipeline* pipeline, RenderPass pass) {
    if (numLines == 0)
        return;

    pipeline->bindVertexShader("DebugLine");
    pipeline->bindVertexSB(sb_lines, 0);
    pipeline->setVertexTopology(VertexTopology::LineList);
    pipeline->drawInstanced(2, numLines);
}

PSDebugDefault::PSDebugDefault() = default;

void PSDebugDefault::bind(Pipeline* pipeline) {
    pipeline->bindPixelShader("DebugLine");
}

} // namespace Graphics
} // namespace Engine