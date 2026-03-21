#pragma once
#pragma once

#include <array>
#include <memory>
#include <queue>
#include <unordered_map>

#include "datamodel/terrain/Terrain.h"

#include "rendering/RenderPass.h"

#include "WaterSurface.h"

namespace Engine {
using namespace Datamodel;

namespace Graphics {
class VisualSystem;
class VisualTerrainImpl;

// VisualTerrain Class:
// Interfaces with the datamodel to pull and generate terrain data for the
// visual system. This primarily consists of mesh data. To reduce the number of
// draw calls, VisualTerrain will dynamically group chunk meshes into a single
// vertex / index buffer.
class VisualTerrain {
  public:
    static std::unique_ptr<VisualTerrain> create(VisualSystem* visual_system,
                                                 Terrain* terrain);
    ~VisualTerrain();

    // Update the octree and pull the most recent terrain meshesS
    void updateAndUploadTerrainData(ID3D11DeviceContext* context,
                                    RenderPassTerrain& pass_terrain,
                                    const Vector3& camera_pos);

    // Return water surface data
    const WaterSurface* getWaterSurface() const;
    float getSurfaceLevel() const;

    // ImGui
    void imGui();

  private:
    std::unique_ptr<VisualTerrainImpl> mImpl;
    VisualTerrain();
};

} // namespace Graphics
} // namespace Engine