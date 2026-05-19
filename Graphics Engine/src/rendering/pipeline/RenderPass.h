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

struct RenderPassSet {
    uint32_t bitset;

    RenderPassSet() { bitset = 0; }
    RenderPassSet(uint32_t _bitset) { bitset = _bitset; }

    void addPass(RenderPass pass) { bitset |= (1 << pass); }
    void removePass(RenderPass pass) { bitset ^= (1 << pass); };
    bool hasPass(RenderPass pass) const { return bitset & (1 << pass); };
};
static_assert(RenderPass::_Count_ < 32);

} // namespace Graphics
} // namespace Engine