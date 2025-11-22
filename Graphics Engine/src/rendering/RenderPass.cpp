#include "RenderPass.h"

#include "datamodel/terrain/TerrainConfig.h"
#include "util/GPUTimer.h"

#include <assert.h>

namespace Engine {
namespace Graphics {
RenderPassData::RenderPassData(ID3D11DeviceContext* context) {
    context->QueryInterface(IID_PPV_ARGS(&annotation));
    assert(annotation);
}

RenderPassShadows::RenderPassShadows(ID3D11Device* device,
                                     ID3D11DeviceContext* context)
    : RenderPassData(context) {}

RenderPassTerrain::RenderPassTerrain(ID3D11Device* device,
                                     ID3D11DeviceContext* context)
    : RenderPassData(context) {
    sb_chunks.initialize(device, TERRAIN_CHUNK_COUNT * TERRAIN_CHUNK_COUNT *
                                     TERRAIN_CHUNK_COUNT);
    sb_indices.initialize(device, 200000 * 3);
    sb_positions.initialize(device, 300000);
    sb_normals.initialize(device, 300000);

    num_active_chunks = 0;
    max_chunk_triangles = 0;
}

RenderPassDefault::MeshInstance::MeshInstance(std::weak_ptr<Mesh> _mesh,
                                              Matrix4 _m_local_to_world) {
    mesh = _mesh;
    m_local_to_world = _m_local_to_world;
}

RenderPassDefault::RenderPassDefault(ID3D11Device* device,
                                     ID3D11DeviceContext* context)
    : RenderPassData(context) {
    meshes.clear();
}

RenderPassScope_Debug::RenderPassScope_Debug(const RenderPassData& pass,
                                             const std::string& name) {
    annotation = pass.annotation;

    const std::wstring wstring = std::wstring(name.begin(), name.end());
    annotation->BeginEvent(wstring.c_str());
}
RenderPassScope_Debug::~RenderPassScope_Debug() { annotation->EndEvent(); }

} // namespace Graphics
} // namespace Engine