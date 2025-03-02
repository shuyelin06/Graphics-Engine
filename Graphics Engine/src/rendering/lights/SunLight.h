#pragma once

#include "Light.h"

constexpr int SUN_NUM_CASCADES = 3;

namespace Engine {
namespace Graphics {
class CameraFrustum;

// SunLight Class:
// Defines a light that is used as the sun in the engine.
// The sun uses cascading shadow maps to achieve shadowing
// of far-away points.
// To create a sun light, pass in an array of length SUN_NUM_CASCADES
// with allocated viewports in the shadow atlas.
class SunLight {
  private:
    ShadowLight* light_cascades[SUN_NUM_CASCADES];

    Quaternion direction;
    int resolution;

  public:
    SunLight(ShadowLight** light_arr, int resolution);
    ~SunLight();

    // Set the sun direction
    void setSunDirection(const Vector3& direction);

    // Given the camera frustum, updates the sun's cascades
    void updateSunCascades(const CameraFrustum& cam_frustum);

  private:
    void updateCascade(int index, float min_z, float max_z,
                       const CameraFrustum& cam_frustum);
};

} // namespace Graphics
} // namespace Engine