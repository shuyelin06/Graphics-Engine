#pragma once

#include "Vector3.h"

namespace Engine {
namespace Math {
// Plane Class:
// Represents a plane as a normal and a distance from the origin.
class Plane {
  private:
    Vector3 normal;
    float distance;

  public:
    Plane(const Vector3& normal);
    Plane(const Vector3& normal, const Vector3& center);
    Plane(const Vector3& normal, float distance);
    ~Plane();

    float distanceTo(const Vector3& point);
    float distanceToSigned(const Vector3& point);
};

} // namespace Math
} // namespace Engine