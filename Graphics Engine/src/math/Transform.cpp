#include "Transform.h"

#include <math.h>

#include "Compute.h"

using namespace std;

namespace Engine {
namespace Math {
// Default Constructor:
// Initializes transform with all properties set to 0
Transform::Transform() {
    position_local = Vector3();
    rotation = Quaternion(Vector3(), 1);
    scale = Vector3(1, 1, 1);
}

// GetPosition:
// Gets an object's position
const Vector3& Transform::getPosition() const { return position_local; }

// SetPosition:
// Changes the transform's position by setting it to given values
void Transform::setPosition(float x, float y, float z) {
    position_local.x = x;
    position_local.y = y;
    position_local.z = z;
}

// OffsetPosition:
// Changes the transform's position by adding given values to it
void Transform::offsetPosition(float x, float y, float z) {
    setPosition(position_local.x + x, position_local.y + y,
                position_local.z + z);
}

// GetRotation:
// Returns an object's rotation
const Quaternion& Transform::getRotation() const { return rotation; }

// Set Viewng Direction: 
// Updates the object's rotation so that the object is facing the direction.
// Assumes that the object's "view" is on the +Z axis, and target is in the object's
// local space.
// TODO: Small bug causes rapid swapping of the rotation at specific directions
void Transform::setViewDirection(const Vector3& direc) {
    const Vector3 direction = direc.unit();

    // Now, convert to spherical coordinates
    const Vector3 spherical_coords = Compute::EulerToSpherical(direction);
    const float theta = spherical_coords.y;
    const float phi = spherical_coords.z;

    // We can now determine our rotation quaternion from this. To convert
    // spherical to euler, we rotate about y by theta, then z by phi.
    const Quaternion y_rotate =
        Quaternion::RotationAroundAxis(Vector3::PositiveY(), theta);
    const Quaternion z_rotate =
        Quaternion::RotationAroundAxis(Vector3::PositiveZ(), phi);

    rotation = z_rotate * y_rotate;
}

// LookAt:
// Sets the viewing direction so that the +Z axis is facing the target point
void Transform::lookAt(const Vector3& target) {
    setViewDirection(target - position_local);
}

// SetRotation:
void Transform::setRotation(const Quaternion& quaternion) {
    rotation = quaternion;
}

// Changes the transform's rotation to theta degrees around some axis in space
void Transform::setRotation(const Vector3& axis, float theta) {
    rotation = Quaternion::RotationAroundAxis(axis, theta);
}

// OffsetRotation
// Changes the transform's rotation by adding given values to it
void Transform::offsetRotation(const Vector3& axis, float theta) {
    const Quaternion newRotation = Quaternion::RotationAroundAxis(axis, theta);
    rotation *= newRotation;
}

// GetScale:
// Get an object's scale
const Vector3& Transform::getScale() const { return scale; }

// SetScale:
// Changes the transform's scale by setting it to given values
void Transform::setScale(float x, float y, float z) {
    scale.x = x;
    scale.y = y;
    scale.z = z;
}

// OffsetScale:
// Changes the transform's scale by adding given values to it
void Transform::offsetScale(float x, float y, float z) {
    setScale(scale.x + x, scale.y + y, scale.z + z);
}

// ForwardVector:
// Returns the (local) forward vector for the transform. This is
// equivalent to the rotated Z-axis.
Vector3 Transform::forward(void) const {
    const Vector4 result = rotationMatrix() * Vector4::PositiveZW();
    return result.xyz();
}

// BackwardVector:
// Returns the (local) backward vector for the transform.
// This is equivalent to the rotated negative Z-axis
Vector3 Transform::backward(void) const { return -forward(); }

// RightVector:
// Returns the (local) right vector for the transform.
// This is equivalent to the rotated X-axis
Vector3 Transform::right(void) const {
    const Vector4 result = rotationMatrix() * Vector4::PositiveXW();
    return result.xyz();
}

// LeftVector
// Returns the (local) left vector for the transform.
// This is equivalent to the rotated negative X-axis.
Vector3 Transform::left(void) const { return -right(); }

// UpVector:
// Returns the (local) up vector for the transform.
// This is equivalent to the rotated Y-axis.
Vector3 Transform::up(void) const {
    Vector4 result = rotationMatrix() * Vector4::PositiveYW();
    return result.xyz();
}

// DownVector:
// Returns the (local) up vector for the transform.
// This is equivalent to the rotated negative Y-axis.
Vector3 Transform::down(void) const { return -up(); }

// TransformMatrix:
// Returns the 4x4 matrix representing the scale, rotation,
// and translations for a given transform
Matrix4 Transform::transformMatrix(void) const {
    // Generate the transformation matrices
    const Matrix4 m_scale = scaleMatrix();             // Scale
    const Matrix4 m_rotation = rotationMatrix();       // Rotation
    const Matrix4 m_translation = translationMatrix(); // Translation

    // Build final matrix
    // Left matrix gets precedence, as we are performing row-major
    // multiplication
    return m_translation * m_rotation * m_scale;
}

// ScaleMatrix:
// Returns the scale matrix for the transform
Matrix4 Transform::scaleMatrix(void) const {
    return Matrix4(scale.x, 0, 0, 0, 0, scale.y, 0, 0, 0, 0, scale.z, 0, 0, 0,
                   0, 1);
};

// RotationMatrix:
// Returns the rotation matrix for the transform
Matrix4 Transform::rotationMatrix(void) const {
    return GenerateRotationMatrix(rotation);
}

// TranslationMatrix:
// Returns the translation matrix for the transform
Matrix4 Transform::translationMatrix(void) const {
    return GenerateTranslationMatrix(position_local.x, position_local.y,
                                     position_local.z);
}

Matrix4 Transform::GenerateTranslationMatrix(float x, float y, float z) {
    return Matrix4(1, 0, 0, x, 0, 1, 0, y, 0, 0, 1, z, 0, 0, 0, 1);
}
Matrix4 Transform::GenerateRotationMatrix(const Quaternion& q) {
    const Vector3 qv = q.im;
    const float qw = q.r;

    // Create quaternion matrix
    const Matrix4 rotation_matrix = Matrix4(
        1 - 2 * (qv.y * qv.y + qv.z * qv.z), 2 * (qv.x * qv.y - qw * qv.z),
        2 * (qv.x * qv.z + qw * qv.y), 0.f, 2 * (qv.x * qv.y + qw * qv.z),
        1 - 2 * (qv.x * qv.x + qv.z * qv.z), 2 * (qv.y * qv.z - qw * qv.x), 0.f,
        2 * (qv.x * qv.z - qw * qv.y), 2 * (qv.y * qv.z + qw * qv.x),
        1 - 2 * (qv.x * qv.x + qv.y * qv.y), 0.f, 0.f, 0.f, 0.f, 1.f);

    return rotation_matrix;
}

Matrix4 Transform::GenerateRotationMatrix(const Vector3& axis, float theta) {
    const Quaternion rotation = Quaternion::RotationAroundAxis(axis, theta);
    return GenerateRotationMatrix(rotation);
}

} // namespace Math
} // namespace Engine