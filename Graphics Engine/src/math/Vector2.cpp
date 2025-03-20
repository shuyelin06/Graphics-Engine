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

// + Operator:
// Returns a new vector which is the summation of
// this vector and another vector
Vector2 Vector2::operator+(const Vector2& vec) const {
    Vector2 vector;
    vector.x = x + vec.x;
    vector.y = y + vec.y;
    return vector;
}

// += Operator
// Adds a vector's x,y,z values to this vector
// in-place
Vector2& Vector2::operator+=(const Vector2& vec) {
    x += vec.x;
    y += vec.y;
    return *this;
}

// - Operator
// Returns a new vector which is the result of this vector
// subtracted with another vector
Vector2 Vector2::operator-(const Vector2& vec) const {
    return Vector2(x - vec.x, y - vec.y);
}

// -= Operator
// Subtracts another vector's x,y,z values from this vector
// in-place
Vector2& Vector2::operator-=(const Vector2& vec) {
    x -= vec.x;
    y -= vec.y;
    return *this;
}

// - Operator (Negate)
// Returns a new vector which is the negation of this vector's
// x,y,z values
Vector2 Vector2::operator-() const { return Vector2(-x, -y); }

} // namespace Math
} // namespace Engine