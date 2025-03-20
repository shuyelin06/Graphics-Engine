#include "VisualDebug.h"

#include <assert.h>

namespace Engine {
namespace Graphics {
// Initializing static fields
std::vector<PointData> VisualDebug::points = std::vector<PointData>();
std::vector<LinePoint> VisualDebug::lines = std::vector<LinePoint>();

// Clear:
// Clears all debug data
void VisualDebug::Clear() { lines.clear(); }

// DrawPoint:
// Registers a point in 3D space to be drawn by the visual
// engine. Points are cleared after every frame
bool VisualDebug::DrawPoint(const Vector3& position, float scale,
                            const Color& color) {
    const int POINT_CAP = (4096 * 4 * sizeof(float)) / sizeof(PointData);

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

        points.push_back(data);

        return true;
    }
}

bool VisualDebug::DrawPoint(const Vector3& position, float scale) {
    return DrawPoint(position, scale, Color::Red());
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

} // namespace Graphics
} // namespace Engine