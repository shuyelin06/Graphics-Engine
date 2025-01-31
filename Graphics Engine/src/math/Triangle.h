#pragma once

#include "Vector3.h"

namespace Engine {
namespace Math {
// Triangle Class:
// Defines a triangle with 3 points: v0, v1, v2.
// Triangles are defined to have a clock-wise winding order.
class Triangle {
  private:
    Vector3 vertices[3];

  public:
    Triangle();
    Triangle(const Vector3& v0, const Vector3& v1, const Vector3& v2);
    ~Triangle();

    const Vector3& vertex(char vertex) const;
    Vector3& vertex(char vertex);

    Vector3 center() const;
    Vector3 normal() const;
};
} // namespace Math
} // namespace Engine