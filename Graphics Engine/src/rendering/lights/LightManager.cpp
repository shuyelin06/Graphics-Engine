#include "LightManager.h"

#include "../ImGui.h"
#include "LightDataGPU.h"
#include "rendering/VisualDebug.h"

namespace Engine
{
namespace Graphics
{
LightManager::LightManager(Device* device, unsigned int atlas_size)
    : shadow_lights()
{
    DMLight::ConnectToCreation([this](Object* obj) { onObjectCreate(obj); });

    atlas_texture = device->createTexture(
        "Shadow Atlas", TextureLayout::R24_UNORM_G8_UINT,
        TextureUsage::DepthStencil | TextureUsage::ShaderResource, atlas_size,
        atlas_size);

    // Create my shadow atlas with this texture
    shadow_atlas = new TextureAtlas(atlas_texture.get());

    // Create sun light
    createSunLight(QUALITY_5);

    ImGuiHelper::registerImGuiCallback("Render/Lighting",
                                       [this]() { imGui(); });
}

// SceneGraph:
void LightManager::pullDatamodelData()
{
    cleanAndPullDatamodelData(shadow_lights);
}

void LightManager::onObjectCreate(Object* object)
{
    if (object->getClassID() == DMLight::ClassID())
    {
        shadow_lights.push_back(createShadowLight(object, QUALITY_5));
    }
}

// --- Update ---
// UpdateTimeOfDay:
// Sets the sun's direction based on the time of day, in hours [0,24].
void LightManager::updateTimeOfDay(float hours_in_day)
{
    constexpr float ANGLE_CONVERSION = 2 * 3.14159f / 24.f;
    const float radians = (hours_in_day - 6.f) * ANGLE_CONVERSION;

    const float x = cosf(radians);
    const float y = sinf(radians);

    sun_light->setSunDirection(Vector3(-x, -y, 0));
}

// UpdateSunDirection:
// Sets the sun direction.
void LightManager::updateSunDirection(const Vector3& direction)
{
    sun_light->setSunDirection(direction);
}

// UpdateSunCascades:
// Uses the camera frustum to set the sun's shadow lights
// so that they properly shadow what the camera sees.
void LightManager::updateSunCascades(const Frustum& camera_frustum)
{
    sun_light->updateSunCascades(camera_frustum);
}

// ResetShadowCasters:
// Clears the shadow caster vector
void LightManager::resetShadowCasters() { shadow_casters.clear(); }

// AddShadowCaster:
// Adds a shadow caster to the light manager
void LightManager::addShadowCaster(const ShadowCaster& caster)
{
    shadow_casters.push_back(caster);
}

// ClusterShadowCasters:
// Clusters the shadow casters so that assets outside of a light's view are
// not rendered
void LightManager::clusterShadowCasters()
{
    shadow_clusters.clear();
    shadow_cluster_indices.clear();

    // For each light, iterate through the assets and find the assets
    // in the light's view. All assets outside the light's view do not need
    // to be ran through the shadow pass
    const std::vector<ShadowLight*>& lights = shadow_lights;
    for (int i = 0; i < lights.size(); i++)
    {
        const ShadowLight* light = lights[i];
        const Frustum frustum = light->frustum();

        ShadowCluster cluster;
        cluster.light_index = i;
        cluster.caster_start = shadow_cluster_indices.size();
        cluster.caster_offset = 0;

        for (int j = 0; j < shadow_casters.size(); j++)
        {
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
const std::shared_ptr<Texture> LightManager::getAtlasTexture(void) const
{
    return atlas_texture;
}

// GetLights:
// Returns the lights.
const SunLight* LightManager::getSunLight() const { return sun_light; }

const ShadowLight* LightManager::getShadowLight(UINT index) const
{
    return shadow_lights[index];
}

const std::vector<ShadowLight*>& LightManager::getShadowLights() const
{
    return shadow_lights;
}

const std::vector<ShadowCluster>& LightManager::getShadowClusters() const
{
    return shadow_clusters;
}
const std::vector<UINT>& LightManager::getShadowClusterIndices() const
{
    return shadow_cluster_indices;
}
const std::vector<ShadowCaster>& LightManager::getShadowCasters() const
{
    return shadow_casters;
}

// CreateShadowLight:
// Creates and returns a shadowed light that can be used in the
// rendering engine.
ShadowLight* LightManager::createShadowLight(Object* object,
                                             ShadowMapQuality quality)
{
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

    ShadowLight* light = new ShadowLight(object, shadow_viewport);
    shadow_lights.push_back(light);

    return light;
}

// CreateSunLight:
// Initializes a sun light object, which uses shadow map cascades.
// Each cascade will have resolution given by the ShadowMapQuality
// parameter.
void LightManager::createSunLight(ShadowMapQuality quality)
{
    ShadowLight* lights[SUN_NUM_CASCADES];
    Object* sun_obj = new Object("Unknown");

    for (int i = 0; i < SUN_NUM_CASCADES; i++)
    {
        ShadowLight* light = createShadowLight(sun_obj, quality);
        lights[i] = light;
    }

    sun_light = new SunLight(lights, quality);
}

// --- Binding ---
// BindLightData:
// Binds lighting data to a provided constant buffer handle.
void LightManager::bindLightData(DeviceContext* context)
{
    const std::vector<ShadowLight*>& lights = shadow_lights;

    std::vector<uint8_t> data;
    auto appendData = [&data](const void* src, size_t bytes) {
        // Convert our data into a character array, and read the number of bytes
        // specified by the CBDataFormat into our constant buffer.
        const char* charData = static_cast<const char*>(src);

        data.resize(data.size() + bytes);
        uint8_t* vectorEnd = &data[data.size() - bytes];

        if (src != nullptr)
            memcpy(vectorEnd, charData, bytes);
        else
            memset(vectorEnd, 0, bytes);
    };

    LightDataGPU lightData;
    // Needed for normalization of the texture coordinates to [0,1].
    const float tex_width = (float)shadow_atlas->getTexture()->getWidth();
    const float tex_height = (float)shadow_atlas->getTexture()->getHeight();

    const int lightCount = lights.size();
    appendData(&lightCount, 4);

    // Global Lighting Data
    const Vector3 sun_direc = sun_light->getDirection();
    appendData(&sun_direc, 12);

    const Vector2 thresholds = Vector2(0.4f, 0.75f);
    appendData(&thresholds, 8);
    appendData(nullptr, 8);

    for (int i = 0; i < SUN_NUM_CASCADES; i++)
    {
        lights[i]->uploadGPUData(lightData);
        lightData.tex_x /= tex_width;
        lightData.tex_y /= tex_height;
        lightData.tex_width /= tex_width;
        lightData.tex_height /= tex_height;
        appendData(&lightData, sizeof(LightDataGPU));
    }

    // Local Lighting Data
    for (int i = SUN_NUM_CASCADES; i < lights.size(); i++)
    {
        lights[i]->uploadGPUData(lightData);
        lightData.tex_x /= tex_width;
        lightData.tex_y /= tex_height;
        lightData.tex_width /= tex_width;
        lightData.tex_height /= tex_height;
        appendData(&lightData, sizeof(LightDataGPU));
    }

    context->loadPixelCB(1, data.data(), data.size());
}

void LightManager::imGui()
{
#if defined(IMGUI_ENABLED)
    ImGui::Text("Number of Lights: %zu", shadow_lights.size());

    static bool show_light_frustums = false;
    ImGui::Checkbox("Show Light Frustums", &show_light_frustums);
    if (show_light_frustums)
    {
        for (auto& light : shadow_lights)
        {
            VisualDebug::DrawFrustum(
                (light->getFrustumMatrix() * light->getWorldMatrix().inverse())
                    .inverse(),
                Color::Green());
        }
    }

    static bool show_atlas = false;
    ImGui::Checkbox("Show Shadow Atlas", &show_atlas);
    if (show_atlas)
    {
        shadow_atlas->getTexture()->doImgui();
    }
#endif
}

} // namespace Graphics
} // namespace Engine