#pragma once

#include "../core/AssetComponent.h"
#include "../core/TextureAtlas.h"
#include "datamodel/ComponentHandler.h"
#include "datamodel/SceneGraph.h"

#include "Light.h"
#include "SunLight.h"

#include <vector>

namespace Engine {
using namespace Datamodel;

namespace Graphics {
// ShadowMapQuality:
// Qualitites that are available for defining the lights with
enum ShadowMapQuality {
    QUALITY_0 = 64,
    QUALITY_1 = 128,
    QUALITY_2 = 256,
    QUALITY_3 = 512,
    QUALITY_4 = 1024,
    QUALITY_5 = 2048,
    QUALITY_DEFAULT = QUALITY_1
};

// Struct NormalizedViewport:
// Normalized shadow map viewport with values in range [0,1].
// Used in the rendering loop.
struct NormalizedShadowViewport {
    float x, y, width, height;
};

// ShadowCluster Struct:
// Represents a group of meshes that should be rendered in the light's shadow
// pass
struct ShadowCluster {
    UINT light_index;

    UINT caster_start;
    UINT caster_offset;
};

struct ShadowCaster {
    const Mesh* mesh;
    Matrix4 m_localToWorld;
};

// LightManager Class:
// Handles creation of lights, both shadowed and unshadowed.
// All shadowed lights use one texture, known as the "shadow_atlas".
// This is one large texture which has dedicated sections
// for different light shadow maps.
class CameraFrustum;
class IConstantBuffer;

class LightManager {
  private:
    TextureAtlas* shadow_atlas;
    std::vector<ShadowLight*> shadow_lights;

    SunLight* sun_light;

    // Information for rendering:
    // shadow_clusters associates lights with the assets in their view, given as
    //      contiguous arrays of indices in shadow_cluster_indices
    // shadow_casters stores what can cast shadows
    std::vector<ShadowCluster> shadow_clusters;
    std::vector<UINT> shadow_cluster_indices;

    std::vector<ShadowCaster> shadow_casters;

  public:
    LightManager(ID3D11Device* device, unsigned int atlas_size);

    // Datamodel Handling 
    void pullDatamodelData();
    void onObjectCreate(Object* object);

    // Get the light manager's data
    const Texture* getAtlasTexture(void) const;

    const SunLight* getSunLight() const;
    const ShadowLight* getShadowLight(UINT index) const;
    const std::vector<ShadowLight*>& getShadowLights() const;

    const std::vector<ShadowCluster>& getShadowClusters() const;
    const std::vector<UINT>& getShadowClusterIndices() const;
    const std::vector<ShadowCaster>& getShadowCasters() const;

    const NormalizedShadowViewport
    normalizeViewport(const ShadowMapViewport viewport) const;

    // Update the light manager
    ShadowLight* createShadowLight(Object* object, ShadowMapQuality quality);

    void updateTimeOfDay(float hours_in_day);
    void updateSunDirection(const Vector3& direction);
    void updateSunCascades(const Frustum& camera_frustum);

    void resetShadowCasters();
    void addShadowCaster(const ShadowCaster& caster);
    void clusterShadowCasters();

    // Bind data to pipeline
    void bindLightData(IConstantBuffer& cb);

  private:
    void createSunLight(ShadowMapQuality quality);
    void bindLight(ShadowLight* light, IConstantBuffer& cb);
};

} // namespace Graphics
} // namespace Engine