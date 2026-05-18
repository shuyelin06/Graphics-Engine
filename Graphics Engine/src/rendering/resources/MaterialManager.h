#pragma once

#include <memory>

namespace Engine {
namespace Graphics {
// MaterialManager Class:
// Manages materials.
// In this engine, a "material" is essentially a configurable PixelTechnique
// that applies to a specific pass.
class MaterialManagerImpl;
class MaterialManager {
  public:
    static std::unique_ptr<MaterialManager> create();
    ~MaterialManager();

  private:
    std::unique_ptr<MaterialManagerImpl> mImpl;
    MaterialManager();
};

} // namespace Graphics
} // namespace Engine