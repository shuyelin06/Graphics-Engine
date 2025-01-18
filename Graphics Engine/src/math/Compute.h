#pragma once

#include "Vector3.h"

namespace Engine {

namespace Math {

#define PI 3.141592653589

// Class Compute
// Provides some utility math functions for use
// throughout the program
class Compute {
  public:
    // Forces value to be within the range [low, high]
    static float Clamp(float value, float low, float high);

    // Generates a random value within a range [low, high]
    static float Random(float low, float high);
    static int Random(int low, int high);

    // Conversions between Euler and Spherical Coordinates
    // Euler Space: (x,y,z) based on the X,Y,Z standard basis axes
    // Spherical Space: (r, theta, phi) where r is radius, theta is angle to z-axis, phi is angle on xy-plane
    static Vector3 SphericalToEuler(const Vector3& spherical);
    static Vector3 SphericalToEuler(float r, float theta, float phi);
    static Vector3 EulerToSpherical(const Vector3& euler);
    static Vector3 EulerToSpherical(float x, float y, float z);
};
} // namespace Math
} // namespace Engine