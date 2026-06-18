#pragma once

#include <type_traits>

namespace Engine {
namespace Graphics {

// SamplerSlot Enum:
// Different samplers the pipeline supports.
enum SamplerType : uint8_t {
    Sampler_Point = 0,
    Sampler_Shadow = 1,
    // Linear = 2,
    // Anisotrophic = 3,
    // Note: Additional samplers can be added here
    SamplerCount
};

} // namespace Graphics
} // namespace Engine