#pragma once

#include "math/Matrix4.h"
#include "math/Vector3.h"

namespace Engine {
using namespace Math;
namespace Graphics {
// LightDataGPU:
// Struct that represents GPU Data for lights.
// Sync with Lighting.hlsli : struct LightData
struct LightDataGPU {
    Vector3 position;
    float pad0;

    Vector3 color;
    float pad1;

    Matrix4 m_view;
    Matrix4 m_projection;

    float tex_x;
    float tex_y;
    float tex_width;
    float tex_height;
};

} // namespace Graphics
} // namespace Engine