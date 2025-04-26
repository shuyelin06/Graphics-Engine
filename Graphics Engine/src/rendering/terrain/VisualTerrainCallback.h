#pragma once

#include <mutex>

#include "datamodel/terrain/TerrainCallback.h"

#include "../Direct3D11.h"
#include "../core/Asset.h"
#include "../resources/AssetBuilder.h"

#include "BufferPool.h"

namespace Engine {
using namespace Datamodel;

namespace Graphics {
// VisualTerrainCallback:
// Interfaces with the terrain datamodel to regenerate chunk meshes
// asynchronously.
class VisualTerrainCallback : public TerrainCallback {
  private:
    ID3D11Device* device;

    Mesh* output_mesh;
    MeshBuilder builder;

    std::vector<MeshVertex> vertices;
    std::vector<MeshTriangle> indices;

    bool dirty;

    // Synchronization
    std::mutex mutex;

  public:
    VisualTerrainCallback();

    void initialize(ID3D11Device* device);

    BufferAllocation* loadMesh(BufferPool& pool);
    Mesh* extractMesh();
    bool isDirty();

    void reloadTerrainData(const TerrainChunk* chunk_data);
};

} // namespace Graphics
} // namespace Engine