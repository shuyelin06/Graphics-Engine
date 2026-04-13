#pragma once

#include "../RenderManager.h"
#include "rendering/lights/LightManager.h"

namespace Engine {
namespace Graphics {
class PSTerrain : public PixelTechnique {
  public:
    PSTerrain();

    // PixelTechnique Implementation
    void bind(Pipeline* pipeline) override;
};

} // namespace Graphics
} // namespace Engine