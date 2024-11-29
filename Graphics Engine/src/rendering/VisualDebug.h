#pragma once

#include "Direct3D11.h"

#include <vector>

#include "rendering/Shader.h"

#include "math/Color.h"
#include "math/Matrix4.h"
#include "math/Vector3.h"

#define ENABLE_DEBUG_VISUALS

namespace Engine {
using namespace Math;

namespace Graphics {
// PointData Struct:
// Contains data for a single point to be rendered (for debugging)
// This data is loaded into a constant buffer for use with instancing
struct PointData {
    Vector3 position;
    float scale;

    Color color;

    // Determines for how many frames the point should persist. If -1,
    // the point should exist indefinitely over the entire program.
    // Unused in the shader, but conveniently pads the line data
    int frameExpiration;
};

// LineData Struct:
// Contains data for a line to be rendered (for debugging)
// This data is loaded into a vertex buffer with a line list format
struct LinePoint {
    Vector3 point;
    Color color;
};

// VisualDebug Class:
// Contains methods that can be called statically
// for convenient debugging purposes
// Note: All debug rendering data is cleared after every frame
class VisualDebug {
    friend class VisualSystem;

  private:
    static std::vector<PointData> points;

    static std::vector<LinePoint> lines;
    static ID3D11Buffer* lineVertexBuffer;

  public:
    // Clear all debug rendering data
    static void Clear();

    // Quick and dirty rendering in 3D space
    static bool DrawPoint(const Vector3& position, float scale,
                          const Color& color, int expiration);
    static bool DrawPoint(const Vector3& position, float scale, int expiration);
    static bool DrawLine(const Vector3& p1, const Vector3& p2,
                         const Color& rgb);
    static bool DrawLine(const Vector3& p1, const Vector3& p2);

    // Rendering for specific features
    static void DrawFrustum(const Matrix4& frustumMatrix, const Color& rgb);

    // Pipeline Management
    static int LoadPointData(CBHandle* cbHandle);
    static int LoadLineData(ID3D11DeviceContext* context, ID3D11Device* device);
};
} // namespace Graphics
} // namespace Engine