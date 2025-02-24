#include "SunLight.h"

#include "../VisualDebug.h"
#include "../core/Camera.h"
#include "math/Compute.h"

namespace Engine {
namespace Graphics {
// Note: Constructor will need to be updated if SUN_NUM_CASCADES changes
SunLight::SunLight(ShadowLight** light_arr)
    : light_cascades{light_arr[0], light_arr[1], light_arr[2]} {

    const Vector3 direc = Vector3(0.25f, -0.75f, 0.25f).unit();

    // Now, convert to spherical coordinates
    const Vector3 spherical_coords = EulerToSpherical(direc);
    const float theta = spherical_coords.y;
    const float phi = spherical_coords.z;

    // We can now determine our rotation quaternion from this. To convert
    // spherical to euler, we rotate about y by theta, then z by phi.
    const Quaternion y_rotate =
        Quaternion::RotationAroundAxis(Vector3::PositiveY(), theta);
    const Quaternion z_rotate =
        Quaternion::RotationAroundAxis(Vector3::PositiveZ(), phi);

    direction = z_rotate * y_rotate;

    light_cascades[0]->setColor(Color::Red());
    light_cascades[1]->setColor(Color::Green());
    light_cascades[2]->setColor(Color(0,0,5));
    // Quaternion::RotationAroundAxis(Vector3::PositiveX(), 3.14159f * 2 / 3);
}
SunLight::~SunLight() = default;

void SunLight::updateSunCascades(const CameraFrustum& camera_frustum) {
    constexpr float EPSILON = 0.015f;
    updateCascade(0, 0.0f, 0.90f + EPSILON, camera_frustum);
    updateCascade(1, 0.90f - EPSILON, 0.975f + EPSILON, camera_frustum);
    updateCascade(2, 0.975f - EPSILON, 0.999f, camera_frustum);
}

void SunLight::updateCascade(int index, float min_z, float max_z,
                             const CameraFrustum& cam_frustum) {
    ShadowLight& light = *light_cascades[index];

    // First, determine the division of the camera frustum we'll be operating on
    Vector3 frustum_points[8] = {
        Vector3(-1, -1, min_z), Vector3(1, -1, min_z),  Vector3(1, 1, min_z),
        Vector3(-1, 1, min_z),  Vector3(-1, -1, max_z), Vector3(1, -1, max_z),
        Vector3(1, 1, max_z),   Vector3(-1, 1, max_z),
    };

    // Transform these frustum points into world space, and based on their
    // location, choose the light's location. This is chosen as the center of
    // the frustum points, offset by some multiple of the sun's direction.
    

    for (int i = 0; i < 8; i++) {
        const Vector3 point = frustum_points[i];
        frustum_points[i] = cam_frustum.toWorldSpace(point);
    }

    const Vector3 direc = direction.rotationMatrix3() * Vector3::PositiveZ();

    Vector3 light_pos = cam_frustum.getCameraPosition();
    light_pos -= direc * (75.f + (index) * 30.f);

    const Matrix4 m_world = Transform::GenerateTranslationMatrix(light_pos) *
                            direction.rotationMatrix4();
    light.setWorldMatrix(m_world);

    // We now want to fit our cascades's orthographic projection
    // so that it contains all of these points.
    // Convert the frustum points into the coordinate system relative
    // to the sun cascade.
    const Matrix4 m_world_to_local = light.getWorldMatrix().inverse();

    for (int i = 0; i < 8; i++) {
        Vector4 point = m_world_to_local * Vector4(frustum_points[i], 1.f);
        point = point / point.w;
        frustum_points[i] = point.xyz();
    }

    // Find the max x,y coordinates. We will keep the view a square to avoid any
    // distortions in the shadowmap (which can cause artifacts)
    float max_x = 0.f;
    float max_y = 0.f;
    float z_max = 0.f;

    for (int i = 0; i < 8; i++) {
        const Vector3& point = frustum_points[i];

        max_x = max(max_x, abs(point.x));
        max_y = max(max_y, abs(point.y));
        z_max = max(z_max, abs(point.z));
    }

    // Based on these coordinates, set the shadow map projection matrix
    light.setOrthogonalFrustum(2 * max(max_x, max_y), 1.f, -25.f, z_max + 25.f);
}

} // namespace Graphics
} // namespace Engine