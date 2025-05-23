#pragma once

namespace Engine {
namespace Graphics {

// VertexDataPin:
// Pins for vertex data. Can be combined with bitwise operators to
// specify an input layout.
enum VertexDataPin {
    PIN_NONE = 0,
    PIN_POSITION = 1 << 1,
    PIN_NORMAL = 1 << 2,
    PIN_TEXTURE = 1 << 3,
    PIN_COLOR = 1 << 4,
    PIN_JOINTS = 1 << 5,
    PIN_WEIGHTS = 1 << 6,
    PIN_INSTANCING = 1 << 7
};

// VertexDataStream:
// Indices of vertex data that are available for the pipeline.
// Each stream is its own vertex buffer that a mesh stores. The indices
// of each stream are aligned (vertex 0 has position at index 0, texture at
// position 0, normal at position 0...)
// We separate into data streams so that it's easier to configure shader inputs.
enum VertexDataStream {
    POSITION = 0, // 3D XYZ Position (3 Floats)
    TEXTURE = 1,  // 2D Texture Coordinates (2 Floats)
    NORMAL = 2,   // 3D Normal Direction (3 Floats)
    COLOR = 3,
    INSTANCE_ID = 4,
    DEBUG_LINE = 5, // Position + RGB Color; Debug Line Rendering
    SV_POSITION = 6,
    // Skinning Properties
    JOINTS = 7,  // 4D Integer Vector of Node Indices (4 Integers)
    WEIGHTS = 8, // 4D Vector of Skin Weights (4 Floats)

    STREAM_COUNT,

};

} // namespace Graphics
} // namespace Engine