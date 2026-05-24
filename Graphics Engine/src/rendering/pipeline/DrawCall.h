#pragma once

#include <stdint.h>
#include <string>
#include <vector>

#include "math/Color.h"

#include "rendering/core/Mesh.h"
#include "rendering/core/Material.h"

namespace Engine {
namespace Graphics {
struct InstanceData {
    Matrix4 localMatrix = Matrix4::Identity();

    Color color = Color(1, 1, 1);
    float _padding = 0;
};

// Stores all data needed for the pipeline to make a draw
// call.
// Ideally we want to move away from virtual calls if we can help it,
// but this will do for simplicity (for now).
// Data is stored in order of what is most --> least important
// when sorting draw calls. We do not want a separate sorting key as
// that would be inefficient.
using InstanceDataKey = uint32_t;
constexpr InstanceDataKey kInvalidInstanceDataKey = 0xFFFF;

struct DrawCall {
    uint32_t depth = 0xFF;

    // Mesh, Technique Replaces both Vertex and Pixel Technique
    const Mesh* mesh = nullptr;
    const Technique* technique = nullptr;

    // Index of the Draw Call's instance data in the global
    // instance data cbuffer.
    uint32_t instanceDataIndex = kInvalidInstanceDataKey;

    DrawCall() = default;
};

} // namespace Graphics
} // namespace Engine