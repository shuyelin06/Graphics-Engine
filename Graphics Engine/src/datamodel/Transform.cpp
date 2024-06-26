#include "Transform.h"

#include <math.h>

/*
TODO:
// Calculate camera's forward viewing vector
	Vector3 Camera::forward()
	{
		// Get rotation matrix
		Matrix4 rotation_matrix = transform.rotationMatrix().tranpose();

		// Camera is by default looking in the +Z axis
		Vector4 view = rotation_matrix * Vector4::PositiveZW();
		return view.toVector3();
	}

	// Calculate camera's right viewing vector
	Vector3 Camera::right()
	{
		// Get rotation matrix
		Matrix4 rotation_matrix = transform.rotationMatrix().tranpose();

		// Camera's left is by default looking in the +X axis
		Vector4 view = rotation_matrix * Vector4::PositiveXW();
		return view.toVector3();
	}
*/
using namespace std;

namespace Engine
{
namespace Datamodel
{
	// Default Constructor:
	// Initializes transform with all properties set to 0
	Transform::Transform()
	{
		position_local = Vector3(0, 0, 0);
		rotation = Vector3(0, 0, 0);
		scale = Vector3(1, 1, 1);
	}

	// GetPosition:
	// Gets an object's position
	const Vector3& Transform::getPosition() const
	{
		return position_local;
	}

	// SetPosition:
	// Changes the transform's position by setting it to given values
	void Transform::setPosition(float x, float y, float z)
	{
		position_local.x = x;
		position_local.y = y;
		position_local.z = z;
	}

	// OffsetPosition:
	// Changes the transform's position by adding given values to it
	void Transform::offsetPosition(float x, float y, float z)
	{
		setPosition(position_local.x + x, position_local.y + y, position_local.z + z);
	}

	// GetRotation:
	// Returns an object's rotation
	const Vector3& Transform::getRotation() const
	{
		return rotation;
	}

	// SetRotation:
	// Changes the transform's rotation by setting it to given values
	void Transform::setRotation(float roll, float yaw, float pitch)
	{
		rotation.x = roll;
		rotation.y = yaw;
		rotation.z = pitch;
	}

	// OffsetRotation
	// Changes the transform's rotation by adding given values to it
	void Transform::offsetRotation(float roll, float yaw, float pitch)
	{
		setRotation(rotation.x + roll, rotation.y + yaw, rotation.z + pitch);
	}

	// GetScale:
	// Get an object's scale
	const Vector3& Transform::getScale() const
	{
		return scale;
	}

	// SetScale:
	// Changes the transform's scale by setting it to given values
	void Transform::setScale(float x, float y, float z)
	{
		scale.x = x;
		scale.y = y;
		scale.z = z;
	}

	// OffsetScale:
	// Changes the transform's scale by adding given values to it
	void Transform::offsetScale(float x, float y, float z)
	{
		setScale(scale.x + x, scale.y + y, scale.z + z);
	}

	// ForwardVector:
	// Returns the (local) forward vector for the transform. This is
	// equivalent to the rotated Z-axis.
	Vector3 Transform::forwardVector(void) const
	{
		Vector4 result = rotationMatrix().tranpose() * Vector4::PositiveZW();
		return result.toVector3();
	}
	
	// BackwardVector:
	// Returns the (local) backward vector for the transform.
	// This is equivalent to the rotated negative Z-axis
	Vector3 Transform::backwardVector(void) const
	{
		return -forwardVector();
	}

	// RightVector:
	// Returns the (local) right vector for the transform.
	// This is equivalent to the rotated X-axis
	Vector3 Transform::rightVector(void) const
	{
		Vector4 result = rotationMatrix().tranpose() * Vector4::PositiveXW();
		return result.toVector3();
	}

	// LeftVector
	// Returns the (local) left vector for the transform.
	// This is equivalent to the rotated negative X-axis.
	Vector3 Transform::leftVector(void) const
	{
		return -rightVector();
	}

	// UpVector:
	// Returns the (local) up vector for the transform. 
	// This is equivalent to the rotated Y-axis.
	Vector3 Transform::upVector(void) const
	{
		Vector4 result = rotationMatrix().tranpose() * Vector4::PositiveYW();
		return result.toVector3();
	}

	// DownVector:
	// Returns the (local) up vector for the transform. 
	// This is equivalent to the rotated negative Y-axis.
	Vector3 Transform::downVector(void) const
	{
		return -upVector();
	}
	
	// TransformMatrix:
	// Returns the 4x4 matrix representing the scale, rotation,
	// and translations for a given transform
	Matrix4 Transform::transformMatrix(void) const
	{
		// Generate the transformation matrices
		Matrix4 m_scale = scaleMatrix();					// Scale
		Matrix4 m_rotation = rotationMatrix();				// Rotation
		Matrix4 m_translation = translationMatrix();		// Translation

		// Build final matrix
		// Left matrix gets precedence, as we are performing row-major multiplication
		return m_scale * m_rotation * m_translation;
	}

	// ScaleMatrix:
	// Returns the scale matrix for the transform
	Matrix4 Transform::scaleMatrix(void) const
	{
		return Matrix4(scale.x, 0, 0, 0,
			0, scale.y, 0, 0,
			0, 0, scale.z, 0,
			0, 0, 0, 1);
	};

	// RotationMatrix:
	// Returns the rotation matrix for the transform
	Matrix4 Transform::rotationMatrix(void) const
	{
		// Cache values to avoid recalculating sine and cosine a lot
		float cos_cache = 0;
		float sin_cache = 0;

		// Rotation about the x-axis (roll)
		cos_cache = cosf(rotation.x);
		sin_cache = sinf(rotation.x);
		Matrix4 roll = Matrix4(
			1, 0, 0, 0,
			0, cos_cache, sin_cache, 0,
			0, -sin_cache, cos_cache, 0,
			0, 0, 0, 1
		);

		// Rotation about the y-axis (pitch)
		cos_cache = cosf(rotation.y);
		sin_cache = sinf(rotation.y);
		Matrix4 pitch = Matrix4(
			cos_cache, 0, -sin_cache, 0,
			0, 1, 0, 0,
			sin_cache, 0, cos_cache, 0,
			0, 0, 0, 1
		);

		// Rotation about the z-axis (yaw)
		cos_cache = cosf(rotation.z);
		sin_cache = sinf(rotation.z);
		Matrix4 yaw = Matrix4(
			cos_cache, sin_cache, 0, 0,
			-sin_cache, cos_cache, 0, 0,
			0, 0, 1, 0,
			0, 0, 0, 1
		);

		return roll * pitch * yaw;
	}
	
	// TranslationMatrix:
	// Returns the translation matrix for the transform
	Matrix4 Transform::translationMatrix(void) const
	{
		return Matrix4(
			1, 0, 0, 0,
			0, 1, 0, 0,
			0, 0, 1, 0,
			position_local.x, position_local.y, position_local.z, 1);
	}
}
}