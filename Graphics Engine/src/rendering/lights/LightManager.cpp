#include "LightManager.h"

namespace Engine {
namespace Graphics {
LightManager::LightManager(TextureAtlas* atlas) : shadow_lights() {
    shadow_atlas = atlas;

    // Create sun light
    createSunLight(QUALITY_5);
}

// Update:
// Updates the light manager. Uses the camera frustum to figure out what
// lights need to be updated, and which lights need to be used in the rendering
// pipeline.
void LightManager::update(const CameraFrustum& camera_frustum) {
    sun_light->updateSunCascades(camera_frustum);
}

// GetShadowAtlas:
// Returns the shadow atlas.
const Texture* LightManager::getAtlasTexture(void) const {
    return shadow_atlas->getTexture();
}

// GetLights:
// Returns the lights.
SunLight* LightManager::getSunLight() { return sun_light; }

ShadowLight* LightManager::getShadowLight(UINT index) {
    return shadow_lights[index];
}

const std::vector<ShadowLight*>& LightManager::getShadowLights() const {
    return shadow_lights;
}

// NormalizeViewport:
// Returns a normalized shadowmap viewport.
const NormalizedShadowViewport
LightManager::normalizeViewport(const ShadowMapViewport viewport) const {
    const UINT tex_width = shadow_atlas->getTexture()->width;
    const UINT tex_height = shadow_atlas->getTexture()->height;

    NormalizedShadowViewport output = {};

    output.x = float(viewport.x) / float(tex_width);
    output.y = float(viewport.y) / float(tex_height);
    output.width = float(viewport.width) / float(tex_width);
    output.height = float(viewport.height) / float(tex_height);

    return output;
}

// CreateShadowLight:
// Creates and returns a shadowed light that can be used in the
// rendering engine.
ShadowLight* LightManager::createShadowLight(ShadowMapQuality quality) {
    // Allocate a spot in the ShadowAtlas for our light
    const UINT alloc_index = shadow_atlas->allocateTexture(quality, quality);
    const AtlasAllocation& allocation =
        shadow_atlas->getAllocation(alloc_index);

    // Initialize and return our light
    ShadowMapViewport shadow_viewport = {};
    shadow_viewport.x = allocation.x;
    shadow_viewport.y = allocation.y;
    shadow_viewport.width = allocation.width;
    shadow_viewport.height = allocation.height;

    ShadowLight* light = new ShadowLight(shadow_viewport);
    shadow_lights.push_back(light);

    return light;
}

// CreateSunLight:
// Initializes a sun light object, which uses shadow map cascades.
// Each cascade will have resolution given by the ShadowMapQuality parameter.
void LightManager::createSunLight(ShadowMapQuality quality) {
    ShadowLight* lights[SUN_NUM_CASCADES];

    for (int i = 0; i < SUN_NUM_CASCADES; i++) {
        ShadowLight* light = createShadowLight(quality);
        lights[i] = light;
    }

    sun_light = new SunLight(lights, quality);
}

} // namespace Graphics
} // namespace Engine