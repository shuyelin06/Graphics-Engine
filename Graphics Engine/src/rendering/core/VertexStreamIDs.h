#pragma once

namespace Engine {
namespace Graphics {

// Indices of Available Vertex Data Streams
// Each stream is its own vertex buffer that a mesh stores. The indices
// of each stream are aligned (vertex 0 has position at index 0, texture at
// position 0, normal at position 0...)
// We separate into data streams so that
// it's easier to configure shader inputs.
enum VertexDataStream {
    POSITION = 0, // 3D XYZ Position (3 Floats)
    TEXTURE = 1,  // 2D Texture Coordinates (2 Floats)
    NORMAL = 2,   // 3D Normal Direction (3 Floats)
    COLOR = 3,
    INSTANCE_ID = 4,
    DEBUG_LINE = 5,   // Position + RGB Color; Debug Line Rendering
    SV_POSITION = 6,

    STREAM_COUNT,
    
};

} // namespace Graphics
} // namespace Engine