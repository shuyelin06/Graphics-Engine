#pragma once

#include <cstdint>

namespace Engine
{
namespace Graphics
{

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
// 2) The VertexAddressors array in MeshBuilder.cpp
enum VertexDataStream : uint16_t
{
    // Pack into groups of 4 floats for better accessing patterns
    PosXYZ_TexU = 0,  // XYZ Position + Texture U Coordinate (4 Floats)
    NormXYZ_TexV = 1, // XYZ Normal + Texture V Coordinate (4 Floats)
    ColorRGBA = 2,    // Color RGBA
    JOINTS = 3,       // 4D Integer Vector of Node Indices (4 Integers)
    WEIGHTS = 4,      // 4D Vector of Skin Weights (4 Floats)
    BINDABLE_STREAM_COUNT,

    // These data streams are not bindable by the engine, and will not
    // be stored in meshes. However, they are still valid
    // vertex streams that have assigned slots
    // INSTANCE_ID: Used for instancing
    INSTANCE_ID = BINDABLE_STREAM_COUNT,
    // VERTEX_ID: Used for vertex pulling
    VERTEX_ID,
    // Position + RGB Color; Debug Line Rendering
    DEBUG_LINE,
};

class VertexLayout
{
  private:
    uint16_t layout_pin;

  public:
    VertexLayout();
    VertexLayout(const VertexLayout& layout);

    void setAllStreams();
    void addVertexStream(VertexDataStream stream);

    bool hasVertexStream(VertexDataStream stream) const;
    bool vertexLayoutSupports(const VertexLayout& layout) const;

    size_t totalStrideSize() const;

    bool operator==(const VertexLayout& layout) const;

    static size_t VertexStreamStride(VertexDataStream stream);
};

} // namespace Graphics
} // namespace Engine