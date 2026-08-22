#pragma once

#include <atomic>
#include <array>
#include <memory>

#include "math/AABB.h"

#include "Buffer.h"
#include "VertexStreamIDs.h"

namespace Engine
{
using namespace Math;
namespace Graphics
{
struct Geometry
{
    std::atomic<bool> ready = false;
    AABB aabb;

    uint32_t indexCount = 0;
    uint32_t vertexCount = 0;

    // Optional. If not provided we will render without an index buffer
    std::shared_ptr<Buffer> indexBuffer = nullptr;
    uint32_t indexOffset = 0;

    std::array<std::shared_ptr<Buffer>, VertexDataStream::BINDABLE_STREAM_COUNT>
        vertexBuffers;
    uint32_t vertexOffset = 0;
};

} // namespace Graphics
} // namespace Engine