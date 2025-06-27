#pragma once

#include <mutex>

#include "datamodel/terrain/TerrainCallback.h"

#include "../Direct3D11.h"
#include "../core/Asset.h"
#include "../resources/AssetBuilder.h"

namespace Engine {
using namespace Datamodel;

namespace Graphics {
// VisualTerrainCallback:
// Interfaces with the terrain datamodel to regenerate chunk meshes
// asynchronously.
class VisualTerrainCallback : public TerrainCallback {
  private:
    MeshBuilder builder;
    MeshBuilder output_builder;

    bool dirty;

    // Synchronization
    std::mutex mutex;

  public:
    VisualTerrainCallback();

    void initialize();

    Mesh* loadMesh(ID3D11DeviceContext* context, MeshPool* pool);
    bool isDirty();

    void reloadTerrainData(const TerrainChunk* chunk_data);
};

} // namespace Graphics
} // namespace Engine