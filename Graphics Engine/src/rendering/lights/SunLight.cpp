#include "SunLight.h"

#include <assert.h>

#include "../VisualDebug.h"
#include "../core/Camera.h"
#include "../util/Frustum.h"
#include "math/Compute.h"

namespace Engine {
namespace Graphics {
// Note: Constructor will need to be updated if SUN_NUM_CASCADES changes
SunLight::SunLight(ShadowLight** light_arr, int _resolution)
    : light_cascades{light_arr[0], light_arr[1], light_arr[2]} {
    setSunDirection(Vector3(0.25f, -0.75f, 0.25f).unit());

    resolution = _resolution;

    // DEBUG: Setting the light color helps us see what cascade the pixel is
    // being shadowed from
    /*
    light_cascades[0]->setColor(Color::Red());
    light_cascades[1]->setColor(Color::Green());
    light_cascades[2]->setColor(Color::Blue());
    */
}
SunLight::~SunLight() = default;

const ShadowLight* SunLight::getSunCascade(int index) const {
    return light_cascades[index];
}

Vector3 SunLight::getDirection() const {
    const Vector3 direc = direction.rotationMatrix3() * Vector3::PositiveZ();
    return direc;
}

void SunLight::updateSunCascades(const Frustum& camera_frustum) {
    constexpr float Z_EPSILON = 0.01f;
    constexpr float DIVISIONS[SUN_NUM_CASCADES + 1] = {0.0f, 0.1f, 0.25f, 1.0f};

    for (int i = 0; i < SUN_NUM_CASCADES; i++) {
        const float z_near = DIVISIONS[i] - Z_EPSILON;
        const float z_far = DIVISIONS[i + 1] + Z_EPSILON;
        updateCascade(i, z_near, z_far, camera_frustum);
    }
}

// SetSunDirection:
// Sets the sun's quaternion so that it points in the directional
void SunLight::setSunDirection(const Vector3& direc) {
    direction = Quaternion::RotationToVector(direc);
}

// UpdateCascade:
// Updates one of the sun-light's cascades.
void SunLight::updateCascade(int index, float min_z, float max_z,
                             const Frustum& cam_frustum) {
    ShadowLight& light = *light_cascades[index];

    // First, determine the division of the camera frustum we'll be operating
    // on. We do this by transforming the normalized viewing cube to world
    // space, and based on the min_z and max_z values, finding our near and far
    // planes.
    Vector3 frustum_points[8] = {
        Vector3(-1, -1, 0), Vector3(1, -1, 0), // Near Plane
        Vector3(1, 1, 0),   Vector3(-1, 1, 0), // Near Plane
        Vector3(-1, -1, 1), Vector3(1, -1, 1), // Far Plane
        Vector3(1, 1, 1),   Vector3(-1, 1, 1), // Far Plane
    };

    // Transform from Viewing Cube -> World Space
    for (int i = 0; i < 8; i++) {
        const Vector3 point = frustum_points[i];
        frustum_points[i] = cam_frustum.toWorldSpace(point);
    }

    // For each near / far point pair, find the direction from one to the other,
    // and translate them so that they are z_min, z_max distance from the
    // camera. While we do this, find the frustum center point.
    Vector3 center_point = Vector3(0.f, 0.f, 0.f);

    for (int i = 0; i < 4; i++) {
        const Vector3 p_near = frustum_points[i];
        const Vector3 p_far = frustum_points[i + 4];
        const Vector3 v_direction = p_far - p_near;

        frustum_points[i] = p_near + v_direction * min_z;
        frustum_points[i + 4] = p_near + v_direction * max_z;

        center_point += frustum_points[i];
        center_point += frustum_points[i + 4];
    }

    center_point = center_point / 8;

    // Find the furthest distance from the center point to the edges.
    // This will define the size of our orthographic projection.
    // We add an epsilon to the projection extents to handle imprecision.
    const float radius = (frustum_points[6] - frustum_points[0]).magnitude();
    constexpr float EXTENT_EPSILON = 0.2f;
    const float extent = radius * (1 + EXTENT_EPSILON);
    light.setOrthogonalFrustum(extent, 1.f, 0.f, 500.f);

    // Now, set the light position and orientation.
    // We set the position so that it "snaps" to the nearest texel coordinate,
    // defined by extent_radius / pixel_resolution, to avoid shadow jittering
    const float texel_distance = extent / resolution;

    const Vector3 direc = direction.rotationMatrix3() * Vector3::PositiveZ();
    Vector3 light_pos = center_point;

    // Offset so at a constant y value
    constexpr float SUN_HEIGHT = 150.f;
    light_pos += direc * ((SUN_HEIGHT - light_pos.y) / direc.y);

    // Snap to nearest texel
    light_pos.x = ((int)(light_pos.x / texel_distance)) * texel_distance;
    light_pos.z = ((int)(light_pos.z / texel_distance)) * texel_distance;

    const Matrix4 m_world =
        Matrix4::T_Translate(light_pos) * direction.rotationMatrix4();
    light.setWorldMatrix(m_world);
}

} // namespace Graphics
} // namespace Engine