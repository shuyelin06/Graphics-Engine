#include "VisualDebug.h"

#include "_VertexStreamIDs_.h"
#include <assert.h>

namespace Engine {
namespace Graphics {
// Initializing static fields
std::vector<PointData> VisualDebug::points = std::vector<PointData>();
std::vector<LinePoint> VisualDebug::lines = std::vector<LinePoint>();
ID3D11Buffer* VisualDebug::lineVertexBuffer = nullptr;

// Clear:
// Clears all debug data
void VisualDebug::Clear() { lines.clear(); }

// DrawPoint:
// Registers a point in 3D space to be drawn by the visual
// engine. Points are cleared after every frame
bool VisualDebug::DrawPoint(const Vector3& position, float scale,
                            const Color& color, int expiration) {
    const int POINT_CAP = (4096 * 4 * sizeof(float)) / sizeof(PointData);

    assert(expiration == -1 || expiration > 0);

    // Check if there is space in the constant buffer for the point. If not
    // fail
    if (points.size() >= POINT_CAP) {
        return false;
    }
    // Otherwise, register point in the array
    else {
        PointData data;
        data.position = position;
        data.scale = scale;
        data.color = color;
        data.frameExpiration = expiration;

        points.push_back(data);

        return true;
    }
}

bool VisualDebug::DrawPoint(const Vector3& position, float scale,
                            int expiration) {
    return DrawPoint(position, scale, Color::Red(), expiration);
}

// DrawLine:
// Registers a line in 3D space to be drawn by the visual engine.
// Like points, lines are cleared after every frame.
bool VisualDebug::DrawLine(const Vector3& p1, const Vector3& p2,
                           const Color& rgb) {
    LinePoint data1;
    data1.point = p1;
    data1.color = rgb;
    lines.push_back(data1);

    LinePoint data2;
    data2.point = p2;
    data2.color = rgb;
    lines.push_back(data2);

    return true;
}

bool VisualDebug::DrawLine(const Vector3& p1, const Vector3& p2) {
    return DrawLine(p1, p2, Color::Red());
}

// DrawFrustum:
// Draws a frustum, given a camera space -> world space matrix.
void VisualDebug::DrawFrustum(const Matrix4& frustumMatrix, const Color& rgb) {
    // Box from (-1, -1, 0) to (1, 1, 1). Represents Direct3D's
    // render space in normalized device coordinates.
    Vector4 cube[8] = {
        Vector4(-1, -1, 0, 1), Vector4(1, -1, 0, 1),  Vector4(1, 1, 0, 1),
        Vector4(-1, 1, 0, 1),  Vector4(-1, -1, 1, 1), Vector4(1, -1, 1, 1),
        Vector4(1, 1, 1, 1),   Vector4(-1, 1, 1, 1),
    };

    // Project the cube back into world coordinates.
    for (int i = 0; i < 8; i++) {
        cube[i] = frustumMatrix * cube[i];
        cube[i] = cube[i] / cube[i].w;
    }

    // Render cube
    DrawLine(cube[0].xyz(), cube[1].xyz(), rgb);
    DrawLine(cube[1].xyz(), cube[2].xyz(), rgb);
    DrawLine(cube[2].xyz(), cube[3].xyz(), rgb);
    DrawLine(cube[3].xyz(), cube[0].xyz(), rgb);

    DrawLine(cube[0].xyz(), cube[4].xyz(), rgb);
    DrawLine(cube[1].xyz(), cube[5].xyz(), rgb);
    DrawLine(cube[2].xyz(), cube[6].xyz(), rgb);
    DrawLine(cube[3].xyz(), cube[7].xyz(), rgb);

    DrawLine(cube[4].xyz(), cube[5].xyz(), rgb);
    DrawLine(cube[5].xyz(), cube[6].xyz(), rgb);
    DrawLine(cube[6].xyz(), cube[7].xyz(), rgb);
    DrawLine(cube[7].xyz(), cube[4].xyz(), rgb);
}

// LoadPointData:
// Loads the point data into a given constant buffer
int VisualDebug::LoadPointData(CBHandle* cbHandle) {
    if (points.size() == 0)
        return 0;

    int head = 0;

    // Load data into the constant buffer handle, while removing points
    // which are expired
    for (int i = 0; i < points.size(); i++) {
        PointData& data = points[i];
        cbHandle->loadData(&data.position, FLOAT3);
        cbHandle->loadData(&data.scale, FLOAT);
        cbHandle->loadData(&data.color, FLOAT3);
        cbHandle->loadData(nullptr, FLOAT);

        if (data.frameExpiration > 0)
            data.frameExpiration -= 1;

        if (data.frameExpiration == -1 || data.frameExpiration > 0) {
            points[head] = data;
            head++;
        }
    }

    points.resize(head);

    return head;
}

// LoadLineData:
// Loads the line data into a vertex buffer, and binds it to
// the pipeline. Returns the number of lines to render.
int VisualDebug::LoadLineData(ID3D11DeviceContext* context,
                              ID3D11Device* device) {
    if (lines.size() == 0)
        return 0;

    if (lineVertexBuffer != nullptr)
        lineVertexBuffer->Release();

    D3D11_BUFFER_DESC buff_desc = {};
    buff_desc.ByteWidth = sizeof(LinePoint) * lines.size();
    buff_desc.Usage = D3D11_USAGE_DEFAULT;
    buff_desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;

    D3D11_SUBRESOURCE_DATA sr_data = {0};
    sr_data.pSysMem = (void*)lines.data();

    device->CreateBuffer(&buff_desc, &sr_data, &lineVertexBuffer);
    assert(lineVertexBuffer != nullptr);

    UINT vertexStride = sizeof(LinePoint);
    UINT vertexOffset = 0;

    context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINELIST);
    context->IASetVertexBuffers(DEBUG_LINE, 1, &lineVertexBuffer, &vertexStride,
                                &vertexOffset);

    return lines.size() * 2;
}
} // namespace Graphics
} // namespace Engine