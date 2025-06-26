#include "LightManager.h"

#include "../ImGui.h"
#include "../pipeline/ConstantBuffer.h"

namespace Engine {
namespace Graphics {
LightManager::LightManager(TextureAtlas* atlas) : shadow_lights() {
    shadow_atlas = atlas;

    // Create sun light
    createSunLight(QUALITY_5);
}

// --- Update ---
// UpdateTimeOfDay:
// Sets the sun's direction based on the time of day, in hours [0,24].
void LightManager::updateTimeOfDay(float hours_in_day) {
    constexpr float ANGLE_CONVERSION = 2 * 3.14159f / 24.f;
    const float radians = (hours_in_day - 6.f) * ANGLE_CONVERSION;

    const float x = cosf(radians);
    const float y = sinf(radians);

    sun_light->setSunDirection(Vector3(-x, -y, 0));
}

// UpdateSunDirection:
// Sets the sun direction.
void LightManager::updateSunDirection(const Vector3& direction) {
    sun_light->setSunDirection(direction);
}

// UpdateSunCascades:
// Uses the camera frustum to set the sun's shadow lights
// so that they properly shadow what the camera sees.
void LightManager::updateSunCascades(const Frustum& camera_frustum) {
    sun_light->updateSunCascades(camera_frustum);
}

// ResetShadowCasters:
// Clears the shadow caster vector
void LightManager::resetShadowCasters() { shadow_casters.clear(); }

// AddShadowCaster:
// Adds a shadow caster to the light manager
void LightManager::addShadowCaster(const ShadowCaster& caster) {
    shadow_casters.push_back(caster);
}

// ClusterShadowCasters:
// Clusters the shadow casters so that assets outside of a light's view are not
// rendered
void LightManager::clusterShadowCasters() {
    shadow_clusters.clear();
    shadow_cluster_indices.clear();

    // For each light, iterate through the assets and find the assets
    // in the light's view. All assets outside the light's view do not need to
    // be ran through the shadow pass
    for (int i = 0; i < shadow_lights.size(); i++) {
        const ShadowLight* light = shadow_lights[i];
        const Frustum frustum = light->frustum();

        ShadowCluster cluster;
        cluster.light_index = i;
        cluster.caster_start = shadow_cluster_indices.size();
        cluster.caster_offset = 0;

        for (int j = 0; j < shadow_casters.size(); j++) {
            const ShadowCaster obj = shadow_casters[j];

            const AABB aabb = obj.mesh->aabb;
            const Matrix4 m_local = obj.m_localToWorld;
            OBB obb = OBB(aabb, m_local);
            // if (frustum.intersectsOBB(obb)) {
            shadow_cluster_indices.push_back(j);
            cluster.caster_offset++;
            // }
        }

        if (cluster.caster_offset > 0)
            shadow_clusters.push_back(cluster);
    }
}

// --- Getters ---
// GetShadowAtlas:
// Returns the shadow atlas.
const Texture* LightManager::getAtlasTexture(void) const {
    return shadow_atlas->getTexture();
}

// GetLights:
// Returns the lights.
const SunLight* LightManager::getSunLight() const { return sun_light; }

const ShadowLight* LightManager::getShadowLight(UINT index) const {
    return shadow_lights[index];
}

const std::vector<ShadowLight*>& LightManager::getShadowLights() const {
    return shadow_lights;
}

const std::vector<ShadowCluster>& LightManager::getShadowClusters() const {
    return shadow_clusters;
}
const std::vector<UINT>& LightManager::getShadowClusterIndices() const {
    return shadow_cluster_indices;
}
const std::vector<ShadowCaster>& LightManager::getShadowCasters() const {
    return shadow_casters;
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

// --- Binding ---
// BindLightData:
// Binds lighting data to a provided constant buffer handle.
void LightManager::bindLightData(IConstantBuffer& cb) {
    const int lightCount = shadow_lights.size();
    cb.loadData(&lightCount, INT);

    // Global Lighting Data
    const Vector3 sun_direc = sun_light->getDirection();
    cb.loadData(&sun_direc, FLOAT3);

    const Vector2 thresholds = Vector2(0.4f, 0.75f);
    cb.loadData(&thresholds, FLOAT2);
    cb.loadData(nullptr, FLOAT2);

    for (int i = 0; i < SUN_NUM_CASCADES; i++) {
        ShadowLight* light = shadow_lights[i];
        bindLight(light, cb);
    }

    // Local Lighting Data
    for (int i = SUN_NUM_CASCADES; i < shadow_lights.size(); i++) {
        ShadowLight* light = shadow_lights[i];
        bindLight(light, cb);
    }
}

void LightManager::bindLight(ShadowLight* light, IConstantBuffer& cb) {
    Vector3 position = light->getPosition();
    cb.loadData(&position, FLOAT3);

    cb.loadData(nullptr, FLOAT);

    const Color& color = light->getColor();
    cb.loadData(&color, FLOAT3);

    cb.loadData(nullptr, INT);

    const Matrix4 m_world_to_local = light->getWorldMatrix().inverse();
    cb.loadData(&m_world_to_local, FLOAT4X4);

    const Matrix4& m_local_to_frustum = light->getFrustumMatrix();
    cb.loadData(&m_local_to_frustum, FLOAT4X4);

    const NormalizedShadowViewport normalized_view =
        normalizeViewport(light->getShadowmapViewport());
    cb.loadData(&normalized_view, FLOAT4);
}

} // namespace Graphics
} // namespace Engine