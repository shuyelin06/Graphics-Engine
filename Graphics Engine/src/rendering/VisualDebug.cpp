#include "VisualDebug.h"

#include <assert.h>

#include <math.h>

constexpr int kLineVerticesPerBatch = 100;
constexpr int kPointsPerBatch = 2048;

namespace Engine
{
namespace Graphics
{
VisualDebug::VisualDebug(Device* device, std::shared_ptr<Geometry> pointMesh)
    : pointMesh(pointMesh)
{
    linePositionBuffer = device->createBuffer(
        "Debug Line Position Buffer", BufferType::Vertex,
        sizeof(Vector4) * kLineVerticesPerBatch, nullptr, true);
    lineColorBuffer = device->createBuffer(
        "Debug Line Color Buffer", BufferType::Vertex,
        sizeof(Vector4) * kLineVerticesPerBatch, nullptr, true);
}
VisualDebug::~VisualDebug() = default;

void VisualDebug::render(DeviceContext* context)
{
    renderLines(context);
    renderPoints(context);

    points.clear();
    linePositions.clear();
    lineColors.clear();
}

void VisualDebug::renderLines(DeviceContext* context)
{
    // Upload kLineVerticesPerBatch of line data and execute the line debug renderer
    assert(linePositions.size() == lineColors.size());
    context->bindShaderProgram("DebugLine", "DebugLine");

    Geometry geometry;
    geometry.vertexBuffers[PosXYZ_TexU] = linePositionBuffer;
    geometry.vertexBuffers[ColorRGBA] = lineColorBuffer;

    size_t head = 0;
    while (head < linePositions.size())
    {
        const size_t batchSize =
            min(kLineVerticesPerBatch, linePositions.size() - head);

        context->updateBuffer(linePositionBuffer, linePositions.data() + head,
                              sizeof(Vector4) * batchSize);
        context->updateBuffer(lineColorBuffer, lineColors.data() + head,
                              sizeof(Vector4) * batchSize);
        geometry.indexCount = batchSize;

        context->draw(&geometry, 1, VertexTopology::LineList);

        head += batchSize;
    }
}

void VisualDebug::renderPoints(DeviceContext* context)
{
    context->bindShaderProgram("DebugPoint", "DebugPoint");

    size_t head = 0;
    while (head < points.size())
    {
        const size_t batchSize = min(kPointsPerBatch, points.size() - head);

        context->loadVertexCB(2, points.data() + head,
                              sizeof(PointData) * batchSize);
        context->draw(pointMesh.get(), batchSize);

        head += batchSize;
    }
}

// DrawPoint:
// Registers a point in 3D space to be drawn by the visual
// engine. Points are cleared after every frame
bool VisualDebug::drawPoint(const Vector3& position,
                            float scale,
                            const Color& color)
{
    const int POINT_CAP = (4096 * 4 * sizeof(float)) / sizeof(PointData);

    // Check if there is space in the constant buffer for the point. If not
    // fail
    if (points.size() >= POINT_CAP)
    {
        return false;
    }
    // Otherwise, register point in the array
    else
    {
        PointData data;
        data.position = position;
        data.scale = scale;
        data.color = color;

        points.push_back(data);

        return true;
    }
}

// DrawLine:
// Registers a line in 3D space to be drawn by the visual engine.
// Like points, lines are cleared after every frame.
bool VisualDebug::drawLine(const Vector3& p1,
                           const Vector3& p2,
                           const Color& rgb)
{
    linePositions.push_back(Vector4(p1, 1.f));
    lineColors.push_back(Vector4(rgb.r, rgb.g, rgb.b, 1.f));

    linePositions.push_back(Vector4(p2, 1.f));
    lineColors.push_back(Vector4(rgb.r, rgb.g, rgb.b, 1.f));

    return true;
}

void VisualDebug::drawBox(const Vector3& box_min, const Vector3& box_max)
{
    // clang-format off
    drawLine(Vector3(box_min.x, box_min.y, box_min.z), Vector3(box_max.x, box_min.y, box_min.z));
    drawLine(Vector3(box_max.x, box_min.y, box_min.z), Vector3(box_max.x, box_max.y, box_min.z));
    drawLine(Vector3(box_max.x, box_max.y, box_min.z), Vector3(box_min.x, box_max.y, box_min.z));
    drawLine(Vector3(box_min.x, box_max.y, box_min.z), Vector3(box_min.x, box_min.y, box_min.z));

    drawLine(Vector3(box_min.x, box_min.y, box_max.z), Vector3(box_max.x, box_min.y, box_max.z));
    drawLine(Vector3(box_max.x, box_min.y, box_max.z), Vector3(box_max.x, box_max.y, box_max.z));
    drawLine(Vector3(box_max.x, box_max.y, box_max.z), Vector3(box_min.x, box_max.y, box_max.z));
    drawLine(Vector3(box_min.x, box_max.y, box_max.z), Vector3(box_min.x, box_min.y, box_max.z));

    drawLine(Vector3(box_min.x, box_min.y, box_min.z), Vector3(box_min.x, box_min.y, box_max.z));
    drawLine(Vector3(box_max.x, box_min.y, box_min.z), Vector3(box_max.x, box_min.y, box_max.z));
    drawLine(Vector3(box_max.x, box_max.y, box_min.z), Vector3(box_max.x, box_max.y, box_max.z));
    drawLine(Vector3(box_min.x, box_max.y, box_min.z), Vector3(box_min.x, box_max.y, box_max.z));
    // clang-format on
}

// DrawFrustum:
// Draws a frustum, given a camera space -> world space matrix.
void VisualDebug::drawFrustum(const Matrix4& frustumMatrix, const Color& rgb)
{
    // Box from (-1, -1, 0) to (1, 1, 1). Represents Direct3D's
    // render space in normalized device coordinates.
    Vector4 cube[8] = {
        Vector4(-1, -1, 0, 1), Vector4(1, -1, 0, 1),  Vector4(1, 1, 0, 1),
        Vector4(-1, 1, 0, 1),  Vector4(-1, -1, 1, 1), Vector4(1, -1, 1, 1),
        Vector4(1, 1, 1, 1),   Vector4(-1, 1, 1, 1),
    };

    // Project the cube back into world coordinates.
    for (int i = 0; i < 8; i++)
    {
        cube[i] = frustumMatrix * cube[i];
        cube[i] = cube[i] / cube[i].w;
    }

    // Render cube
    drawLine(cube[0].xyz(), cube[1].xyz(), rgb);
    drawLine(cube[1].xyz(), cube[2].xyz(), rgb);
    drawLine(cube[2].xyz(), cube[3].xyz(), rgb);
    drawLine(cube[3].xyz(), cube[0].xyz(), rgb);

    drawLine(cube[0].xyz(), cube[4].xyz(), rgb);
    drawLine(cube[1].xyz(), cube[5].xyz(), rgb);
    drawLine(cube[2].xyz(), cube[6].xyz(), rgb);
    drawLine(cube[3].xyz(), cube[7].xyz(), rgb);

    drawLine(cube[4].xyz(), cube[5].xyz(), rgb);
    drawLine(cube[5].xyz(), cube[6].xyz(), rgb);
    drawLine(cube[6].xyz(), cube[7].xyz(), rgb);
    drawLine(cube[7].xyz(), cube[4].xyz(), rgb);
}

} // namespace Graphics
} // namespace Engine