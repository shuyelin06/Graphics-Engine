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

    Vector2 unit() const;
    float magnitude() const;

    float dot(const Vector2& vector) const;

    // Returns some vector orthogonal to this vector
    Vector2 orthogonal() const;

    // Vector Operations
    Vector2 operator+(const Vector2&) const; // Addition
    Vector2& operator+=(const Vector2&);     // Compound (In-Place) Addition
    Vector2 operator-(const Vector2&) const; // Subtraction
    Vector2& operator-=(const Vector2&);     // Compound (In-Place) Subtraction
    Vector2 operator-() const;               // Negation
};
} // namespace Math
} // namespace Engine