#pragma once

#include <stdint.h>
#include <string>
#include <variant>

#include "math/CFrame.h"
#include "math/Matrix4.h"

#include "core/StackString.h"

namespace Engine {
namespace Datamodel {
enum class DMEventType {
    kCreated = 0,
    kDestroyed = 1,
    kSelected = 2,
    kPropertyUpdated = 3,
};

using DMObjectHandle = uint32_t;
using DMObjectTag = StackString<16>;
using DMPropertyTag = StackString<16>;

// clang-format off
using DMPropertyData = std::variant
<
    float, 
    uint32_t, 
    Math::Matrix4, 
    std::string
>;
// clang-format on

struct DMEvent {
    DMEventType event_type;

    DMObjectHandle object;
    DMObjectTag object_type;

    // If event_type == Property_Updated
    DMPropertyTag property_tag;
    DMPropertyData property_data;
};

} // namespace Datamodel
} // namespace Engine