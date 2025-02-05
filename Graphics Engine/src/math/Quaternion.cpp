#include "Quaternion.h"

#include <math.h>

namespace Engine {
namespace Math {
Quaternion::Quaternion() : im(0,0,0), r(1) {}

// Vector Constructor:
// Creates a quaternion with given imaginary vector
// and real value component
Quaternion::Quaternion(const Vector3& _im, float _r) {
    im = _im;
    r = _r;
}

// Norm:
// Calculates and returns the conjugate's norm
float Quaternion::norm() const {
    float sq_prod = im.x * im.x + im.y * im.y + im.z * im.z + r * r;
    return sqrtf(sq_prod);
}

// Conjugate:
// Returns this quaternion's conjugate, the quaternion such that
// its product with this gives us a real number
Quaternion Quaternion::conjugate() const {
    Quaternion new_qhat;
    new_qhat.im = -im;
    new_qhat.r = r;
    return new_qhat;
}

// RotationMatrix:
// Generates the rotation matrix for this quaternion. Assumes this quaternion
// is a unit quaternion
Matrix4 Quaternion::rotationMatrix4() const {
    const Vector3 qv = im;
    const float qw = r;

    const Matrix4 rotation_matrix = Matrix4(
        1 - 2 * (qv.y * qv.y + qv.z * qv.z), 2 * (qv.x * qv.y - qw * qv.z),
        2 * (qv.x * qv.z + qw * qv.y), 0.f, 2 * (qv.x * qv.y + qw * qv.z),
        1 - 2 * (qv.x * qv.x + qv.z * qv.z), 2 * (qv.y * qv.z - qw * qv.x), 0.f,
        2 * (qv.x * qv.z - qw * qv.y), 2 * (qv.y * qv.z + qw * qv.x),
        1 - 2 * (qv.x * qv.x + qv.y * qv.y), 0.f, 0.f, 0.f, 0.f, 1.f);

    return rotation_matrix;
}

Matrix3 Quaternion::rotationMatrix3() const {
    const Vector3 qv = im;
    const float qw = r;

    const Matrix3 rotation_matrix = Matrix3(
        1 - 2 * (qv.y * qv.y + qv.z * qv.z), 2 * (qv.x * qv.y - qw * qv.z),
        2 * (qv.x * qv.z + qw * qv.y), 2 * (qv.x * qv.y + qw * qv.z),
        1 - 2 * (qv.x * qv.x + qv.z * qv.z), 2 * (qv.y * qv.z - qw * qv.x),
        2 * (qv.x * qv.z - qw * qv.y), 2 * (qv.y * qv.z + qw * qv.x),
        1 - 2 * (qv.x * qv.x + qv.y * qv.y));

    return rotation_matrix;
}

/* Quaternion Operations */
// Addition:
// Adds two quaternions together and returns a new quaternion
// representing their result
Quaternion Quaternion::operator+(const Quaternion& qhat) const {
    Quaternion new_qhat;
    new_qhat.im = im + qhat.im;
    new_qhat.r = r + qhat.r;
    return new_qhat;
}

// Compound (In-Place) Addition
// Adds one quaternion to the other, where this quaternion is
// modified in-place
Quaternion& Quaternion::operator+=(const Quaternion& qhat) {
    im += qhat.im;
    r += qhat.r;
    return *this;
}

// Multiplication
// Takes the product of two quaternions, and returns
// the result of this product as a new quaternion
// When working with unit quaternions, this is equivalent to
// combining two rotations, where the rightmost quaternion (rotation)
// is applied first.
Quaternion Quaternion::operator*(const Quaternion& qhat) const {
    Quaternion new_qhat;
    new_qhat.im = im.cross(qhat.im) + qhat.im * r + im * qhat.r;
    new_qhat.r = r * qhat.r - im.dot(qhat.im);
    return new_qhat;
}

// Compound (In-Place) Multiplication
// Multiplies a quaternion to this, modifying this quaternion
// in place
Quaternion& Quaternion::operator*=(const Quaternion& qhat) {
    const Vector3 imNew = im.cross(qhat.im) + qhat.im * r + im * qhat.r;
    const float rNew = r * qhat.r - im.dot(qhat.im);
    im = imNew;
    r = rNew;
    return *this;
}

// Identity:
// Returns the identity quaternion, with a 0 imaginary vector
// and real component equal to 1
Quaternion Quaternion::Identity() { return Quaternion(Vector3(0, 0, 0), 1); }

// RotationAroundAxis:
// Generate a unit quaternion representing a rotation around a given axis
Quaternion Quaternion::RotationAroundAxis(const Vector3& axis, float theta) {
    const float radians = theta / 2;
    return Quaternion(axis * sinf(radians), cosf(radians));
}

// RotationBetweenVectors:
// Generate a unit quaternion representing a rotation from one vector to
// another.
// - We can find an axis of rotation as the cross product between the two
// vectors
// - We can find the angle of rotation by taking acos(dot(u,v) / ||u|| ||v||)
Quaternion Quaternion::RotationToVector(const Vector3& from,
                                        const Vector3& to) {
    // TODO: Does not work the best
    const Vector3 fromUnit = from.unit();
    const Vector3 toUnit = to.unit();

    // Axis of rotation is the cross product between the two vectors
    const Vector3 axis = -fromUnit.cross(toUnit).unit();

    // Angle of rotation can be derived from the formula dot(u,v) = cos(theta)
    // ||u|| ||v||
    const float theta =
        atanf(fromUnit.cross(toUnit).magnitude() / fromUnit.dot(toUnit)) / 2;

    return RotationAroundAxis(axis, theta);
}
} // namespace Math
} // namespace Engine