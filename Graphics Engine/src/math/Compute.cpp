#include "Compute.h"

#include <math.h>

#include <assert.h>
#include <stdlib.h>
#include <time.h>

#include "Vector2.h"
#include "Vector4.h"

#include <assert.h>

namespace Engine {

namespace Math {

// Clamp:
// Forces value to be within the range [low, high]
float Clamp(float val, float low, float high) {
    const float temp = val < low ? low : val;
    return temp > high ? high : temp;
}

// Lerp:
// Linearly interpolate between a,b given t.
float Lerp(float a, float b, float t) {
    assert(0 <= t && t <= 1);
    return a * (1 - t) + b * t;
}

// Random:
// Generates a random value within the range [low, high]
float Random(float low, float high) {
    // Generate random float
    float rand_num = (float)(rand()) / RAND_MAX;
    return rand_num * (high - low) + low;
}
int Random(int low, int high) {
    float rand = Random(0.0f, 1.0f);
    return low + (int)(rand * (high - low));
}

// Spherical to Euler Coordinate-System Conversions
Vector3 SphericalToEuler(const Vector3& spherical) {
    return SphericalToEuler(spherical.x, spherical.y, spherical.z);
}
Vector3 SphericalToEuler(float r, float theta, float phi) {
    const float x = r * sinf(theta) * cosf(phi);
    const float y = r * sinf(theta) * sinf(phi);
    const float z = r * cosf(theta);

    return Vector3(x, y, z);
}

Vector3 EulerToSpherical(const Vector3& euler) {
    return EulerToSpherical(euler.x, euler.y, euler.z);
}
Vector3 EulerToSpherical(float x, float y, float z) {
    const float r = sqrtf(x * x + y * y + z * z);
    const float theta = acosf(z / r);
    const float phi = atan2f(y, x);

    return Vector3(r, theta, phi);
}

} // namespace Math
} // namespace Engine