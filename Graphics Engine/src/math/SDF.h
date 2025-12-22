#pragma once

#include "Vector3.h"

// This header file contains many SDF helper functions that can be used
// throughout the code. https://iquilezles.org/articles/distfunctions/ For every
// function, "point" is the position of our input point, and all other parameters
// (prepended by p_, for "parameter") toggle the underlying SDF shape.

namespace Engine {
namespace Math {
inline float SDFSphere(Vector3 point, float p_radius) {
    return point.magnitude() - p_radius;
}

} // namespace Math
} // namespace Engine