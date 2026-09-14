#pragma once

#include <stdint.h>
#include <string>
#include <vector>

#include "math/Color.h"

#include "rendering/core/Material.h"
#include "rendering/core/Mesh.h"

namespace Engine
{
namespace Graphics
{
struct InstanceData
{
    Matrix4 mLocalToWorld = Matrix4::Identity();
    Matrix4 mNormalTransform = Matrix4::Identity();

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
inline constexpr InstanceDataKey kIdentityInstanceDataKey = 0;

struct DrawCall
{
    uint32_t depth = 0xFF;

    // Mesh, Technique Replaces both Vertex and Pixel Technique
    const Geometry* mesh = nullptr;
    const Technique* technique = nullptr;

    // Index of the Draw Call's instance data in the global
    // instance data cbuffer. Default identity
    uint32_t instanceDataIndex = kIdentityInstanceDataKey;

    DrawCall() = default;

    // Comparison is used to sort draw calls for batching
    // Sorting done in this order:
    // 1) Depth first, for correctness during alpha blending
    // 2) Technique next, to minimize rebindings of shader resources
    // 3) Mesh last, to minimize rebindings of vertex / index buffers
    bool operator<(const DrawCall& other) const
    {
        if (depth != other.depth) // Back to front
            return depth > other.depth;
        else if (technique != other.technique)
            return technique < other.technique;
        return mesh < other.mesh;
    }
    // Equality operator for batching
    bool operator==(const DrawCall& other) const
    {
        return depth == other.depth && mesh == other.mesh &&
               technique == other.technique;
    }
};

} // namespace Graphics
} // namespace Engine