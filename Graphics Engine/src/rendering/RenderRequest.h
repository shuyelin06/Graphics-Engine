#pragma once

#include "AssetIDs.h"
#include "math/Matrix4.h"

namespace Engine {
using namespace Math;
namespace Graphics {
// RenderRequest Class(es):
// Structures that represent render requests that are submitted
// to the visual system.
struct AssetRenderRequest {
    AssetSlot slot;
    Matrix4 mLocalToWorld;

    AssetRenderRequest(AssetSlot slot, const Matrix4& mLocalToWorld);
};
} // namespace Graphics
} // namespace Engine
