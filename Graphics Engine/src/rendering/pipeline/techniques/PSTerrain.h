#pragma once

#include "../RenderManager.h"
#include "rendering/lights/LightManager.h"

namespace Engine {
namespace Graphics {
class PSTerrain : public PixelTechnique {
  private:
    LightManager* mLightManager;

  public:
    PSTerrain(LightManager* lightManager);

    // PixelTechnique Implementation
    void bind(Pipeline* pipeline, ID3D11DeviceContext* context) override;
};

} // namespace Graphics
} // namespace Engine