#pragma once

#include <memory>

#include "../DrawCall.h"

#include "rendering/VisualDebug.h"

namespace Engine {
namespace Graphics {
class VSDebugRenderLine : VertexTechnique {
  private:
    StructuredBuffer<LinePoint> sb_lines;
    uint32_t numLines;

  public:
    VSDebugRenderLine();

    void initialize(ID3D11Device* device);
    void uploadData(ID3D11DeviceContext* context,
                    const std::vector<LinePoint>& data);

    // VertexTechnique Implementation
    void bindAndDraw(Pipeline* pipeline, RenderPass pass) override;
};

class VSDebugRenderPoint : VertexTechnique {
  private:
    std::shared_ptr<Mesh> cubeMesh;

    std::vector<PointData>* pointData;

  public:
    VSDebugRenderPoint();

    void initialize(std::shared_ptr<Mesh> cubeMesh);
    void uploadData(ID3D11DeviceContext* context, std::vector<PointData>& data);

    // VertexTechnique Implementation
    void bindAndDraw(Pipeline* pipeline, RenderPass pass) override;
};

class PSDebugDefault : PixelTechnique {
  public:
    PSDebugDefault();

    // PixelTechnique Implementation
    void bind(Pipeline* pipeline) override;
};

} // namespace Graphics
} // namespace Engine