#pragma once

#include "Vector3.h"
#include "Matrix3.h"
#include "Matrix4.h"

namespace Engine {
namespace Math {
// Quaternion Class:
// Represents a quaternion, used to represent rotations in 3D space.
// Quaternions are given in the form
// xi + yj + zk + r = q
// Where i,j,k are imaginary components.
// If we express quaternions in the form
// (sin(theta) * axis, cos(theta)), we can use them
// to represent a rotation around the axis in space.
class Quaternion {
  private:
    // Imaginary Component
    Vector3 im;

    // Real component
    float r;

    // Other quaternions can only be constructed through static
    // methods, to ensure that they stay unit.
    Quaternion(const Vector3& im, float real);

  public:
    // Creates the identity quaternion
    Quaternion();

    float norm() const;
    Quaternion conjugate() const;

    // Generate Rotation Matrices (must be unit quaternion)
    Matrix4 rotationMatrix4() const;
    Matrix3 rotationMatrix3() const;

    // Quaternion Operations
    Quaternion operator+(const Quaternion&) const; // Addition
    Quaternion& operator+=(const Quaternion&); // Compound (In-Place) Addition
    Quaternion operator*(const Quaternion&) const; // Product
    Quaternion& operator*=(const Quaternion&); // Compound (In-Place) Product

    // Identity Quaternion
    static Quaternion Identity();

    // Generate a unit quaternion representing a rotation around a given axis
    static Quaternion RotationAroundAxis(const Vector3& axis, float theta);

    // Generate a unit quaternion representing a rotation that rotates +Z to some direction
    static Quaternion RotationToVector(const Vector3& direction);
};
} // namespace Math
} // namespace Engine