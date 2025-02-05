#pragma once

#include "Vector3.h"

namespace Engine {
namespace Math {
class Matrix4;

// Vector4
// Contains methods and data for a 4-dimensional
// vector.
class Vector4 {
  public:
    float x, y, z, w;

    Vector4();
    Vector4(const Vector3& vec3, float w);
    Vector4(const Vector4& copy);
    Vector4(float _x, float _y, float _z, float _w);

    ~Vector4();

    void normalize();

    float magnitude();
    Vector3 xyz() const; // Drops the w term

    Vector4 operator*(Matrix4&) const; // Row-Major Multiplication
    Vector4 operator+(const Vector4&) const;
    Vector4 operator-(const Vector4&) const;
    Vector4 operator*(const Vector4&) const;
    Vector4 operator/(const Vector4&) const;

    Vector4 operator/(float) const;

    static Vector4 PositiveXW(); // (1,0,0,1)
    static Vector4 PositiveYW(); // (0,1,0,1)
    static Vector4 PositiveZW(); // (0,0,1,1)
};

} // Namespace Math
} // Namespace Engine