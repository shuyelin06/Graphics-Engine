#pragma once

#include "Vector3.h"

namespace Engine {

namespace Math {

#define PI 3.141592653589

// Class Compute
// Provides some utility math functions for use
// throughout the program
// Forces value to be within the range [low, high]
float Clamp(float value, float low, float high);

// Linearly Interpolate between a,b with t in [0,1]. If t = 0, return a. If t =
// 1, return b.
float Lerp(float a, float b, float t);

// Generates a random value within a range [low, high]
float Random(float low, float high);
int Random(int low, int high);

// Conversions between Euler and Spherical Coordinates
// Euler Space: (x,y,z) based on the X,Y,Z standard basis axes
// Spherical Space: (r, theta, phi) where r is radius, theta is angle to z-axis,
// phi is angle on xy-plane
Vector3 SphericalToEuler(const Vector3& spherical);
Vector3 SphericalToEuler(float r, float theta, float phi);
Vector3 EulerToSpherical(const Vector3& euler);
Vector3 EulerToSpherical(float x, float y, float z);

} // namespace Math
} // namespace Engine