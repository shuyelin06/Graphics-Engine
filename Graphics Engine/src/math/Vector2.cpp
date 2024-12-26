#include "Vector2.h"

#include <math.h>

namespace Engine {
namespace Math {
// Constructors and Destructor:
Vector2::Vector2() : x(0), y(0) {}

Vector2::Vector2(const Vector2& copy) : x(copy.x), y(copy.y) {}

Vector2::Vector2(float _x, float _y) : x(_x), y(_y) {}

Vector2::~Vector2() = default;

// Unit:
// Normalizes the vector. Assumes the vector is not a 0-vector
Vector2 Vector2::unit() const {
    const float length = magnitude();
    return Vector2(x / length, y / length);
}
// Magnitude:
// Returns the magnitude (length) of the vector.
float Vector2::magnitude() const {
    return sqrtf(x * x + y * y);
}

// Dot:
// Calculates the dot product between two 2D vectors.
float Vector2::dot(const Vector2& vector) const {
    return x * vector.x + y * vector.y;
}

// Orthogonal:
// Returns some vector orthogonal to this vector
Vector2 Vector2::orthogonal() const {
    return Vector2(y, -x);
}

} // namespace Math
} // namespace Engine