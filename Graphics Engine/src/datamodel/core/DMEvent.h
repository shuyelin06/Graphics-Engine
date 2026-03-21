#pragma once

#include <stdint.h>
#include <string>
#include <variant>

namespace Engine {
namespace Datamodel {
enum class DMEventType {
    kCreated = 0,
    kDestroyed = 1,
    kSelected = 2,
    kPropertyUpdated = 3,
};

using DMObjectHandle = uint32_t;
// clang-format off
enum class DMObjectTag : uint16_t{
    kUnknown = 0,
    kCamera = 1,
    kLight = 2,
    kMesh = 3,
    kPhysics = 4,
    kTerrain = 5,
};
// clang-format on

// clang-format off
enum class DMPropertyTag {
    // Transform
    kPosition, kRotation, kScale,
    // Camera
    kCamera_FOV, kCamera_ZNear, kCamera_ZFar,

};
// clang-format on

constexpr size_t kDMPropertyMaxSize = 16;
using DMPropertyData = uint8_t[kDMPropertyMaxSize];

struct DMEvent {
    DMEventType event_type;

    DMObjectHandle object;
    DMObjectTag object_type;

    // If event_type == Property_Updated
    DMPropertyTag property_tag;
    DMPropertyData property_data;

    template <typename T> T& pullPropertyData() {
        static_assert(sizeof(T) <= kDMPropertyMaxSize);
        return *reinterpret_cast<T*>(property_data);
    }
};

template <typename T>
T& pullPropertyData(const DMPropertyData& data) {
    static_assert(sizeof(T) <= kDMPropertyMaxSize);
    return *reinterpret_cast<T*>(data);
}

} // namespace Datamodel
} // namespace Engine