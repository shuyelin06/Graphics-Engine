#include "TerrainMesh.h"

#include <algorithm>

#include "datamodel/terrain/TerrainConfig.h"

#include "rendering/core/Texture.h"

namespace Engine {
namespace Graphics {
TerrainMesh::TerrainMesh() = default;

void TerrainMesh::initialize(ID3D11Device* device,
                             ID3D11DeviceContext* context) {
    constexpr int MAX_CHUNK_COUNT =
        TERRAIN_CHUNK_COUNT * TERRAIN_CHUNK_COUNT * TERRAIN_CHUNK_COUNT;
    sb_descriptors.initialize(device, MAX_CHUNK_COUNT);
    sb_indices.initialize(device, 200000 * 3);
    sb_positions.initialize(device, 300000);
    sb_normals.initialize(device, 300000);

    num_active_chunks = 0;
    max_chunk_triangles = 0;
}

void TerrainMesh::uploadDescriptors(
    ID3D11DeviceContext* context,
    const std::vector<TerrainMesh::MeshDescription>& descriptors) {
    num_active_chunks = descriptors.size();
    max_chunk_triangles = 0;

    for (const auto& descriptor : descriptors) {
        max_chunk_triangles =
            max(max_chunk_triangles, descriptor.index_count / 3);
    }

    sb_descriptors.uploadData(context, descriptors.data(), descriptors.size());
}
void TerrainMesh::uploadIndices(ID3D11DeviceContext* context, void* addr,
                                size_t numElements) {
    sb_indices.uploadData(context, addr, numElements);
}
void TerrainMesh::uploadPositions(ID3D11DeviceContext* context, void* addr,
                                  size_t numElements) {
    sb_positions.uploadData(context, addr, numElements);
}
void TerrainMesh::uploadNormals(ID3D11DeviceContext* context, void* addr,
                                size_t numElements) {
    sb_normals.uploadData(context, addr, numElements);
}

void TerrainMesh::bindAndExecute(Pipeline* pipeline,
                                 ID3D11DeviceContext* context) {
    pipeline->bindVertexShader("Terrain");
    pipeline->bindPixelShader("Terrain");

    // TEST
    Texture* depth_stencil = pipeline->getDepthStencil();
    pipeline->bindRenderTarget(Target_UseExisting, Depth_TestAndWrite,
                               Blend_Default);
    depth_stencil->clearAsDepthStencil(context);

    /*
    // Vertex Constant Buffer 0:
    // Stores the camera view and projection matrices
    {
        IConstantBuffer vCB0 = pipeline->loadVertexCB(CB0);
        vCB0.loadData(&cache->m_world_to_screen, FLOAT4X4);
    }

    // Pixel Constant Buffer 1: Light Data
    // Stores data that is needed for lighting / shadowing.
    {
        IConstantBuffer pCB1 = pipeline->loadPixelCB(CB1);
        light_manager->bindLightData(pCB1);
    }
    */

    context->IASetIndexBuffer(NULL, DXGI_FORMAT_R32_UINT, 0);

    sb_descriptors.VSBindResource(context, 0);
    sb_indices.VSBindResource(context, 1);
    sb_positions.VSBindResource(context, 2);
    sb_normals.VSBindResource(context, 3);

    // We draw instanced without indices, so the index buffer has no influence
    // on the final result.
    const int num_chunks = num_active_chunks;
    const int max_tris = max_chunk_triangles;
    context->DrawInstanced(max_tris * 3, num_chunks, 0, 0);
}

} // namespace Graphics
} // namespace Engine