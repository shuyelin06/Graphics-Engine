#include "Vector3.h"

#include <algorithm>
#include <math.h>

namespace Engine {
namespace Math {

// Default Constructor:
// Creates a vector whose x,y,z values are all 0
Vector3::Vector3() { x = y = z = 0.f; }

// Copy Constructor:
// Creates a vector which is a copy of the
// vector passed in
Vector3::Vector3(const Vector3& copy) {
    x = copy.x;
    y = copy.y;
    z = copy.z;
}

// Constructor:
// Creates a vector given input x,y,z values
Vector3::Vector3(float _x, float _y, float _z) {
    x = _x;
    y = _y;
    z = _z;
}

// Set:
// Updates the contents of the current vector
void Vector3::set(const Vector3& vec) {
    x = vec.x;
    y = vec.y;
    z = vec.z;
} 

// XZY:
// Returns a new vector with the components rearranged
Vector3 Vector3::xzy() const {
    return Vector3(x,z,y);
}

// Magnitude:
// Returns the vector's magnitude
float Vector3::magnitude() const { return sqrtf(x * x + y * y + z * z); }

// Unit:
// Returns a unit vector. Does not modify this vector.
Vector3 Vector3::unit() const {
    const float length = magnitude();
    return Vector3(x / length, y / length, z / length);
}

// InplaceNormalize:
// Normalizes the vector inplace
void Vector3::inplaceNormalize() {
    const float length = magnitude();
    x /= length;
    y /= length;
    z /= length;
}

// Dot:
// Performs the dot product between two vectors.
float Vector3::dot(const Vector3& vector) const {
    return x * vector.x + y * vector.y + z * vector.z;
}

// Cross:
// Performs the cross product between two vectors.
Vector3 Vector3::cross(const Vector3& vector) const {
    return Vector3(y * vector.z - z * vector.y, -(x * vector.z - z * vector.x),
                   x * vector.y - y * vector.x);
}

// ProjectOnto:
// Projects this vector onto another vector.
Vector3 Vector3::projectOnto(const Vector3& vector) const {
    const float dot_product = dot(vector);
    return vector * dot_product;
}

// Min:
// Returns the component-wise minimum.
Vector3 Vector3::componentMin(const Vector3& vector) const {
    const Vector3 result(std::min(x, vector.x), std::min(y, vector.y),
                         std::min(z, vector.z));
    return result;
}

// Max:
// Returns the component-wise maximum.
Vector3 Vector3::componentMax(const Vector3& vector) const {
    const Vector3 result(std::max(x, vector.x), std::max(y, vector.y),
                         std::max(z, vector.z));
    return result;
}

// + Operator:
// Returns a new vector which is the summation of
// this vector and another vector
Vector3 Vector3::operator+(const Vector3& vec) const {
    Vector3 vector;
    vector.x = x + vec.x;
    vector.y = y + vec.y;
    vector.z = z + vec.z;
    return vector;
}

// += Operator
// Adds a vector's x,y,z values to this vector
// in-place
Vector3& Vector3::operator+=(const Vector3& vec) {
    x += vec.x;
    y += vec.y;
    z += vec.z;
    return *this;
}

// - Operator
// Returns a new vector which is the result of this vector
// subtracted with another vector
Vector3 Vector3::operator-(const Vector3& vec) const {
    return Vector3(x - vec.x, y - vec.y, z - vec.z);
}

// -= Operator
// Subtracts another vector's x,y,z values from this vector
// in-place
Vector3& Vector3::operator-=(const Vector3& vec) {
    x -= vec.x;
    y -= vec.y;
    z -= vec.z;
    return *this;
}

// - Operator (Negate)
// Returns a new vector which is the negation of this vector's
// x,y,z values
Vector3 Vector3::operator-() const { return Vector3(-x, -y, -z); }

// * Operator
// Returns a new vector which is the result of this vector
// scalar multiplied by some value
Vector3 Vector3::operator*(const float f) const {
    Vector3 vector;
    vector.x = x * f;
    vector.y = y * f;
    vector.z = z * f;
    return vector;
}

// *= Operator
// Multiplies this vector's values by a scalar
// value in-place
Vector3& Vector3::operator*=(const float f) {
    x *= f;
    y *= f;
    z *= f;
    return *this;
}

// / Operator
// Returns a new vector which is the result of this vector
// divided by a scalar value
Vector3 Vector3::operator/(const float f) const {
    Vector3 vector;
    vector.x = x / f;
    vector.y = y / f;
    vector.z = z / f;
    return vector;
}

// /= Operator
// Divides this vector's values by a scalar value in-place
// Assumes f is not 0.
Vector3& Vector3::operator/=(const float f) {
    x /= f;
    y /= f;
    z /= f;
    return *this;
}

// * Operator (Hamming Product) 
// Takes the component wise product of the two vectors.
Vector3 Vector3::operator*(const Vector3& v) const
{
    Vector3 result;
    result.x = x * v.x;
    result.y = y * v.y;
    result.z = z * v.z;
    return result;
}

// *= Operator (Hamming)
// Takes the compound component wise product of the two vectors.
Vector3& Vector3::operator*=(const Vector3& v)
{
    x = x * v.x;
    y = y * v.y;
    z = z * v.z;
    return *this;
}

// Static Vector Operations:
// Statically create vectors
Vector3 Vector3::PositiveX() { return Vector3(1, 0, 0); }

Vector3 Vector3::PositiveY() { return Vector3(0, 1, 0); }

Vector3 Vector3::PositiveZ() { return Vector3(0, 0, 1); }

Vector3 Vector3::NegativeX() { return Vector3(-1, 0, 0); }

Vector3 Vector3::NegativeY() { return Vector3(0, -1, 0); }

Vector3 Vector3::NegativeZ() { return Vector3(0, 0, -1); }

Vector3 Vector3::VectorMax() { return Vector3(FLT_MAX, FLT_MAX, FLT_MAX); }

Vector3 Vector3::VectorMin() { return Vector3(-FLT_MAX, -FLT_MAX, -FLT_MAX); }

} // Namespace Math
} // Namespace Engine