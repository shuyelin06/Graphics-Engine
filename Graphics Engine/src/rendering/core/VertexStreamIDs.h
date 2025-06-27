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
//
// IMPORTANT:
// If this is modified, the following must also be updated:
// 1) The StreamStrides array in VertexStreamIDs.cpp
// 2) The VertexAddressors array in AssetBuilder.cpp
enum VertexDataStream {
    POSITION = 0, // 3D XYZ Position (3 Floats)
    TEXTURE = 1,  // 2D Texture Coordinates (2 Floats)
    NORMAL = 2,   // 3D Normal Direction (3 Floats)
    COLOR = 3,    // RGB Color (3 Floats)
    JOINTS = 4,   // 4D Integer Vector of Node Indices (4 Integers)
    WEIGHTS = 5,  // 4D Vector of Skin Weights (4 Floats)

    BINDABLE_STREAM_COUNT,

    // These data streams are not bindable by the engine, and will not
    // be stored in meshes. However, they are still valid
    // vertex streams that have assigned slots
    // INSTANCE_ID: Used for instancing
    INSTANCE_ID = BINDABLE_STREAM_COUNT,
    // VERTEX_ID: Used for vertex pulling
    VERTEX_ID,
    // SV_POSITION: Used for post-processing
    SV_POSITION,
    // Position + RGB Color; Debug Line Rendering
    DEBUG_LINE,
};

bool LayoutPinHas(uint16_t pin, unsigned int stream);
unsigned int StreamVertexStride(unsigned int stream);

uint16_t VertexStreamLayoutPin(VertexDataStream* streams, size_t size);

} // namespace Graphics
} // namespace Engine