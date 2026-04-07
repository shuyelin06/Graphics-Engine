#include "VSTerrain.h"

#include <algorithm>

#include "datamodel/terrain/TerrainConfig.h"

#include "rendering/core/Texture.h"

namespace Engine {
namespace Graphics {
VSTerrain::VSTerrain() = default;

void VSTerrain::initialize(ID3D11Device* device, ID3D11DeviceContext* context) {
    constexpr int MAX_CHUNK_COUNT =
        TERRAIN_CHUNK_COUNT * TERRAIN_CHUNK_COUNT * TERRAIN_CHUNK_COUNT;
    sb_descriptors.initialize(device, MAX_CHUNK_COUNT);
    sb_indices.initialize(device, 200000 * 3);
    sb_positions.initialize(device, 300000);
    sb_normals.initialize(device, 300000);

    num_active_chunks = 0;
    max_chunk_triangles = 0;
}

void VSTerrain::uploadDescriptors(
    ID3D11DeviceContext* context,
    const std::vector<VSTerrain::MeshDescription>& descriptors) {
    num_active_chunks = descriptors.size();
    max_chunk_triangles = 0;

    for (const auto& descriptor : descriptors) {
        max_chunk_triangles =
            max(max_chunk_triangles, descriptor.index_count / 3);
    }

    sb_descriptors.uploadData(context, descriptors.data(), descriptors.size());
}
void VSTerrain::uploadIndices(ID3D11DeviceContext* context, void* addr,
                              size_t numElements) {
    sb_indices.uploadData(context, addr, numElements);
}
void VSTerrain::uploadPositions(ID3D11DeviceContext* context, void* addr,
                                size_t numElements) {
    sb_positions.uploadData(context, addr, numElements);
}
void VSTerrain::uploadNormals(ID3D11DeviceContext* context, void* addr,
                              size_t numElements) {
    sb_normals.uploadData(context, addr, numElements);
}

void VSTerrain::bindAndDraw(Pipeline* pipeline, PipelineRenderPass pass) {
    pipeline->bindVertexShader("Terrain");

    pipeline->bindVertexSB(sb_descriptors, 0);
    pipeline->bindVertexSB(sb_indices, 1);
    pipeline->bindVertexSB(sb_positions, 2);
    pipeline->bindVertexSB(sb_normals, 3);

    // We draw instanced without indices, so the index buffer has no influence
    // on the final result.
    pipeline->drawInstanced(max_chunk_triangles * 3, num_active_chunks);
}

} // namespace Graphics
} // namespace Engine