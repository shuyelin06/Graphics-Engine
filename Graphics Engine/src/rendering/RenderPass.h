#pragma once

#include "math/Vector3.h"

#include "Direct3D11.h"
#include "core/Geometry.h"
#include "pipeline/StructuredBuffer.h"
#include <d3d11_1.h>

#include <string>

#define RENDER_PASS(pass, name)                                                 \
    RenderPassScope_Debug renderpass_debug = RenderPassScope_Debug(pass, name); \
    IGPUTimer gpu_timer = GPUTimer::TrackGPUTime(name);

namespace Engine {
using namespace Math;

namespace Graphics {

// Render Pass Data Structures.
// Visual Engine uses these to execute the render passes. Various subsystems
// should fill in these buffers with information accordingly. All of them
// inherit from a base "RenderPass" struct for debug information (e.g. RenderDoc
// annotations)
struct RenderPassData {
    ID3DUserDefinedAnnotation* annotation;

    RenderPassData(ID3D11DeviceContext* context);
};

struct RenderPassShadows : public RenderPassData {

    RenderPassShadows(ID3D11Device* device, ID3D11DeviceContext* context);
};

struct RenderPassTerrain : public RenderPassData {
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

    RenderPassTerrain(ID3D11Device* device, ID3D11DeviceContext* context);
};

struct RenderPassDefault : public RenderPassData {
    struct GeometryInstance {
        std::shared_ptr<Geometry> geometry;
        Matrix4 m_local_to_world;

        GeometryInstance(std::shared_ptr<Geometry> mesh, Matrix4 m_local_to_world);
    };

    std::vector<GeometryInstance> meshes;

    RenderPassDefault(ID3D11Device* device, ID3D11DeviceContext* context);
};

// RenderPassScope_Debug:
// Responsible for RenderDoc annotations when executing a Render Pass.
// Should be created at the beginning of the pass, and automatically ends the
// event on destruction
class RenderPassScope_Debug {
    ID3DUserDefinedAnnotation* annotation;

  public:
    RenderPassScope_Debug(const RenderPassData& pass, const std::string& name);
    ~RenderPassScope_Debug();
};

} // namespace Graphics
} // namespace Engine