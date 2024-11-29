#pragma once

namespace Engine {
namespace Math {
// Vector2 Class:
// Contains methods and data for a 2-dimensional
// vector2.
class Vector2 {
  public:
    // x,u and v,y components are equivalent, and can be
    // used interchangeably.
    union {
        float u;
        float x;
    };
    union {
        float v;
        float y;
    };

    Vector2();
    Vector2(const Vector2& copy);
    Vector2(float _x, float _y);

    ~Vector2();

    float dot(const Vector2& vector) const;
};
} // namespace Math
} // namespace Engine