#pragma once
#pragma once

#include <array>
#include <memory>
#include <queue>
#include <unordered_map>
#include <variant>

#include "WaterSurface.h"

namespace Engine {
namespace Graphics {
class VisualSystem;
class VisualTerrainImpl;

// VisualTerrain Class:
// Interfaces with the datamodel to pull and generate terrain data for the
// visual system. This primarily consists of mesh data. To reduce the number of
// draw calls, VisualTerrain will dynamically group chunk meshes into a single
// vertex / index buffer.
class TerrainManager {
  public:
    static std::unique_ptr<TerrainManager> create(VisualSystem* visual_system);
    ~TerrainManager();

    // Scene Updating
    struct UpdatePacket {
        enum class Type : uint8_t {
            kToggleTerrain,
            kPropertySeed,
        };
        Type type;
        std::variant<bool, uint32_t> data;
    };
    void submitSceneUpdate(const UpdatePacket& packet);

    // Update the octree and pull the most recent terrain meshesS
    void updateAndUploadTerrainData(ID3D11DeviceContext* context,
                                    const Vector3& camera_pos);

    // Return water surface data
    const WaterSurface* getWaterSurface() const;
    float getSurfaceLevel() const;

    // ImGui
    void imGui();

  private:
    std::unique_ptr<VisualTerrainImpl> mImpl;
    TerrainManager();
};

} // namespace Graphics
} // namespace Engine