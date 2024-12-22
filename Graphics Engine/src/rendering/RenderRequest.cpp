#include "RenderRequest.h"

namespace Engine {
namespace Graphics {
AssetRenderRequest::AssetRenderRequest(AssetSlot _slot,
                                       const Matrix4& _mLocalToWorld) {
    slot = _slot;
    mLocalToWorld = _mLocalToWorld;
}
} // namespace Graphics
} // namespace Engine
