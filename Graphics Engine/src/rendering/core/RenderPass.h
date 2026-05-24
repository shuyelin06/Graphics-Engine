#pragma once

#include <stdint.h>

namespace Engine {
namespace Graphics {
enum RenderPass {
    kShadows = 0,
    kOpaque = 1,
    kDebug = 2,
    _Count_,
};

} // namespace Graphics
} // namespace Engine