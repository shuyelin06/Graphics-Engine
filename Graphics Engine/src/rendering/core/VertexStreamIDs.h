#pragma once

#include <cstdint>

namespace Engine {
namespace Graphics {

// VertexDataStream:
// Indices of vertex data that are available for the pipeline.
// Each stream is its own vertex buffer that a mesh stores. The indices
// of each stream are aligned (vertex 0 has position at index 0, texture at
// position 0, normal at position 0...)
// We separate into data streams so that it's easier to configure shader inputs.
//
// These stream indices can be converted to a layout pin, by treating each
// index as a bit position. For example, if a layout needs POSITION, TEX, then
// flip the bits at POSITION (0) and TEXTURE (1).
enum VertexDataStream {
    POSITION = 0, // 3D XYZ Position (3 Floats)
    TEXTURE = 1,  // 2D Texture Coordinates (2 Floats)
    NORMAL = 2,   // 3D Normal Direction (3 Floats)
    COLOR = 3,
    DEBUG_LINE = 4, // Position + RGB Color; Debug Line Rendering
    // Skinning Properties
    JOINTS = 5,  // 4D Integer Vector of Node Indices (4 Integers)
    WEIGHTS = 6, // 4D Vector of Skin Weights (4 Floats)

    BINDABLE_STREAM_COUNT,

    // These data streams are not bindable by the engine, and will not
    // be stored in meshes. However, they are still valid
    // vertex streams that have assigned slots
    // INSTANCE_ID: Used for instancing
    INSTANCE_ID = BINDABLE_STREAM_COUNT,
    // SV_POSITION: Used for post-processing
    SV_POSITION,
};

static inline bool LayoutPinHas(uint16_t pin, VertexDataStream stream) {
    return (pin & (1 << stream)) == (1 << stream);
}

static inline uint16_t VertexStreamLayoutPin(VertexDataStream* streams,
                                             size_t size) {
    uint16_t pin = 0;

    for (int i = 0; i < size; i++)
        pin |= (1 << streams[i]);

    return pin;
}

} // namespace Graphics
} // namespace Engine