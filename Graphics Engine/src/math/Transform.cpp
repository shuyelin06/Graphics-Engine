#include "Transform.h"

#include <math.h>

#include "Compute.h"

using namespace std;

namespace Engine {
namespace Math {
// Default Constructor:
// Initializes transform with all properties set to 0
Transform::Transform() {
    position_local = Vector3(0, 0, 0);
    rotation = Quaternion::Identity();
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

void Transform::setPosition(const Vector3& pos) {
    setPosition(pos.x, pos.y, pos.z);
}

// OffsetPosition:
// Changes the transform's position by adding given values to it
void Transform::offsetPosition(float x, float y, float z) {
    position_local.x = position_local.x + x;
    position_local.y = position_local.y + y;
    position_local.z = position_local.z + z;
}

void Transform::offsetPosition(const Vector3& offset) {
    offsetPosition(offset.x, offset.y, offset.z);
}

// GetRotation:
// Returns an object's rotation
const Quaternion& Transform::getRotation() const { return rotation; }

// Set Viewng Direction:
// Updates the object's rotation so that the object is facing the direction.
// Assumes that the object's "view" is on the +Z axis, and target is in the
// object's local space.
// TODO: Small bug causes rapid swapping of the rotation at specific directions
void Transform::setViewDirection(const Vector3& direc) {
    const Vector3 direction = direc.unit();

    // Now, convert to spherical coordinates
    const Vector3 spherical_coords = EulerToSpherical(direction);
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

void Transform::setScale(const Vector3& scale) {
    setScale(scale.x, scale.y, scale.z);
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
    const Matrix4 m_scale =
        Matrix4::T_Scale(scale.x, scale.y, scale.z);       // Scale
    const Matrix4 m_rotation = rotation.rotationMatrix4(); // Rotation
    const Matrix4 m_translation =
        Matrix4::T_Translate(position_local); // Translation

    // Build final matrix
    // Left matrix gets precedence, as we are performing row-major
    // multiplication
    return m_translation * m_rotation * m_scale;
}

// ScaleMatrix:
// Returns the scale matrix for the transform
Matrix4 Transform::scaleMatrix(void) const {
    return Matrix4(scale.x, 0, 0, 0, // R1
                   0, scale.y, 0, 0, // R2
                   0, 0, scale.z, 0, // R3
                   0, 0, 0, 1);      // R4
};

// RotationMatrix:
// Returns the rotation matrix for the transform
Matrix4 Transform::rotationMatrix(void) const {
    return rotation.rotationMatrix4();
}

} // namespace Math
} // namespace Engine