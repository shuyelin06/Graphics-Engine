#include "Compute.h"

#include <math.h>

#include <assert.h>
#include <stdlib.h>
#include <time.h>

#include "Transform.h"
#include "Vector2.h"
#include "Vector4.h"

namespace Engine {

namespace Math {

// Clamp:
// Forces value to be within the range [low, high]
float Compute::Clamp(float val, float low, float high) {
    const float temp = val < low ? low : val;
    return temp > high ? high : temp;
}

// Random:
// Generates a random value within the range [low, high]
float Compute::Random(float low, float high) {
    // Generate random float
    float rand_num = (float)(rand()) / RAND_MAX;
    return rand_num * (high - low) + low;
}
int Compute::Random(int low, int high) {
    float rand = Compute::Random(0.0f, 1.0f);
    return low + (int) (rand * (high - low));
}

// Spherical to Euler Coordinate-System Conversions
Vector3 Compute::SphericalToEuler(const Vector3& spherical) {
    return SphericalToEuler(spherical.x, spherical.y, spherical.z);
}
Vector3 Compute::SphericalToEuler(float r, float theta, float phi) {
    const float x = r * sinf(theta) * cosf(phi);
    const float y = r * sinf(theta) * sinf(phi);
    const float z = r * cosf(theta);

    Vector4 z_axis = Vector4(0, 0, r, 1);
    Quaternion q1 = Quaternion::RotationAroundAxis(Vector3::PositiveY(), theta);
    Quaternion q2 = Quaternion::RotationAroundAxis(Vector3::PositiveZ(), phi);
    z_axis = Transform::GenerateRotationMatrix(q2 * q1) * z_axis;

    return Vector3(x, y, z);
}

Vector3 Compute::EulerToSpherical(const Vector3& euler) {
    return EulerToSpherical(euler.x, euler.y, euler.z);
}
Vector3 Compute::EulerToSpherical(float x, float y, float z) {
    const float r = sqrtf(x * x + y * y + z * z);
    const float theta = acosf(z / r);
    const float phi = atan2f(y, x);

    return Vector3(r, theta, phi);
}

} // namespace Math
} // namespace Engine