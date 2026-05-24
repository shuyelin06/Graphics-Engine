#pragma once

#include <array>
#include <memory>
#include <string>
#include <vector>

#include "rendering/core/Material.h"

namespace Engine {
namespace Graphics {
class MaterialManagerImpl;

// MaterialManager Class:
// Manages materials.
// In this engine, a "material" is essentially a configurable PixelTechnique
// that applies to a specific pass.
class MaterialManager {
  public:
    struct DefaultMaterialParams {
        std::string colormap;

        bool debug;
    };
    struct TerrainMaterialParams {};

    static std::unique_ptr<MaterialManager> create();
    ~MaterialManager();

    std::shared_ptr<Material>
    createMaterial(const DefaultMaterialParams& params);
    std::shared_ptr<Material>
    createMaterial(const TerrainMaterialParams& params);

  private:
    std::unique_ptr<MaterialManagerImpl> mImpl;
    MaterialManager();
};

} // namespace Graphics
} // namespace Engine