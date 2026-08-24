#pragma once

#include <vector>

#include "math/Color.h"
#include "math/Matrix4.h"
#include "math/Vector3.h"

#include "rendering/core/Device.h"

namespace Engine
{
using namespace Math;

namespace Graphics
{
// PointData Struct:
// Contains data for a single point to be rendered (for debugging)
// This data is loaded into a constant buffer for use with instancing
struct PointData
{
    Vector3 position;
    float scale;

    Color color;
    float padding;
};

// VisualDebug Class:
// Contains methods that can be called statically
// for convenient debugging purposes
// Note: All debug rendering data is cleared after every frame
class VisualDebug
{
  private:
    std::vector<PointData> points;
    std::shared_ptr<Geometry> pointMesh;

    std::vector<Vector4> linePositions;
    std::vector<Vector4> lineColors;
    std::shared_ptr<Buffer> linePositionBuffer;
    std::shared_ptr<Buffer> lineColorBuffer;

  public:
    VisualDebug(Device* device, std::shared_ptr<Geometry> pointMesh);
    ~VisualDebug();

    void render(DeviceContext* context);

    // Quick and dirty rendering in 3D space
    bool drawPoint(const Vector3& position,
                   float scale,
                   const Color& rgb = Color::Red());

    bool drawLine(const Vector3& p1,
                  const Vector3& p2,
                  const Color& rgb = Color::Red());
    void drawBox(const Vector3& box_min, const Vector3& box_max);
    void drawFrustum(const Matrix4& frustumMatrix, const Color& rgb);

  private:
    void renderLines(DeviceContext* context);
    void renderPoints(DeviceContext* context);
};
} // namespace Graphics
} // namespace Engine