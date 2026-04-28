#include "DebugRenderTech.h"

#include "rendering/core/Mesh.h"

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

VSDebugRenderPoint::VSDebugRenderPoint() = default;

void VSDebugRenderPoint::initialize(std::shared_ptr<Mesh> cubeMesh) {
    this->cubeMesh = cubeMesh;
}

void VSDebugRenderPoint::uploadData(ID3D11DeviceContext* context,
                                    std::vector<PointData>& data) {
    this->pointData = &data;
}

// VertexTechnique Implementation
void VSDebugRenderPoint::bindAndDraw(Pipeline* pipeline, RenderPass pass) {
    if (pointData->size() == 0)
        return;

    pipeline->bindVertexShader("DebugPoint");
    pipeline->bindRenderTarget(Target_UseExisting, Depth_TestNoWrite,
                               Blend_UseSrcAndDest);

    ID3D11Buffer* indexBuffer = cubeMesh->buffer_pool->ibuffer;
    ID3D11Buffer* vertexBuffer = cubeMesh->buffer_pool->vbuffers[POSITION];
    int numIndices = cubeMesh->num_triangles * 3;

    UINT vertexStride = sizeof(float) * 3;
    UINT vertexOffset = 0;

    ID3D11DeviceContext* context = pipeline->getContext();
    context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    context->IASetVertexBuffers(POSITION, 1, &vertexBuffer, &vertexStride,
                                &vertexOffset);
    context->IASetIndexBuffer(indexBuffer, DXGI_FORMAT_R32_UINT, 0);

    const int numPoints = pointData->size();

    // Load data into the constant buffer handle, while removing points
    // which are expired
    {
        IConstantBuffer vCB0 = pipeline->loadVertexCB(CB2);

        for (int i = 0; i < pointData->size(); i++) {
            const PointData& data = (*pointData)[i];
            vCB0.loadData(&data.position, FLOAT3);
            vCB0.loadData(&data.scale, FLOAT);
            vCB0.loadData(&data.color, FLOAT3);
            vCB0.loadData(nullptr, FLOAT);
        }
    }

    pointData->clear();

    if (numPoints > 0) {
        context->DrawIndexedInstanced(numIndices, numPoints, 0, 0, 1);
    }
}

PSDebugDefault::PSDebugDefault() = default;

void PSDebugDefault::bind(Pipeline* pipeline) {
    pipeline->bindPixelShader("DebugLine");
}

} // namespace Graphics
} // namespace Engine