#pragma once

#include "math/Vector3.h"

#include "Direct3D11.h"
#include "pipeline/StructuredBuffer.h"

namespace Engine {
using namespace Math;

namespace Graphics {

struct RenderPassTerrain {
    struct TerrainChunkDescription {
        unsigned int index_start;
        unsigned int index_count;

        unsigned int vertex_start;
        unsigned int vertex_count;
    };

    StructuredBuffer<TerrainChunkDescription> sb_chunks;
    StructuredBuffer<unsigned int> sb_indices;
    StructuredBuffer<Vector3> sb_positions;
    StructuredBuffer<Vector3> sb_normals;

    int num_active_chunks;
    int max_chunk_triangles;

    RenderPassTerrain(ID3D11Device* device);
};

} // namespace Graphics
} // namespace Engine