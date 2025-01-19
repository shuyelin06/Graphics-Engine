#pragma once

#include "Light.h"
#include "TextureAtlas.h"

#include <vector>

namespace Engine {
namespace Graphics {
enum ShadowMapQuality {
    QUALITY_0 = 64,
    QUALITY_1 = 128,
    QUALITY_2 = 256,
    QUALITY_3 = 512,
    QUALITY_DEFAULT = QUALITY_1
};

// Struct NormalizedViewport:
// Normalized shadow map viewport with values in range [0,1].
// Used in the rendering loop.
struct NormalizedShadowViewport {
    float x, y, width, height;
};

// LightManager Class:
// Handles creation of lights, both shadowed and unshadowed.
// All shadowed lights use one texture, known as the "shadow_atlas".
// This is one large texture which has dedicated sections
// for different light shadow maps.
class LightManager {
  private:
    TextureAtlas* shadow_atlas;
    std::vector<ShadowLight*> shadow_lights;

  public:
    LightManager(TextureAtlas* shadow_atlas);

    const Texture* getAtlasTexture(void) const;
    
    ShadowLight* getShadowLight(UINT index);
    const std::vector<ShadowLight*>& getShadowLights() const;

    const NormalizedShadowViewport
    normalizeViewport(const ShadowMapViewport viewport) const;

    // Light creation
    ShadowLight* createShadowLight(ShadowMapQuality quality);
};

} // namespace Graphics
} // namespace Engine