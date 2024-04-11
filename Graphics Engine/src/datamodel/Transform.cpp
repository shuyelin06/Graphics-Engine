#include "Transform.h"

#include <math.h>

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
		rotation = Quaternion::Identity();
		scale = Vector3(1, 1, 1);
	}

	// GetPosition:
	// Gets an object's position
	const Vector3 Transform::getPosition() const
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
	const Quaternion Transform::getRotation() const
	{
		return rotation;
	}

	// SetRotation:
	// Changes the transform's rotation given euler angles
	void Transform::setRotation(float roll, float pitch, float yaw)
	{
		rotation = Quaternion(roll, pitch, yaw);
	}

	// SetRotation:
	// Changes the transform's rotation by setting it to given values
	void Transform::setRotation(const Vector3& axis, float theta)
	{
		// Rotations with quaternions have an angle of 2 * theta,
		// so we divide by 2
		theta /= 2; 
		rotation = Quaternion(axis * sinf(theta), cosf(theta));
	}

	// OffsetRotation
	// Changes the transform's rotation by adding given values to it
	void Transform::offsetRotation(const Vector3& axis, float theta)
	{
		// Rotations with quaternions have an angle of 2 * theta,
		// so we divide by 2
		theta /= 2;
		Quaternion new_rotation = Quaternion(axis * sinf(theta), cosf(theta));
		Quaternion final_rotate = new_rotation * rotation * new_rotation;
		rotation = final_rotate;
	}

	// GetScale:
	// Get an object's scale
	const Vector3 Transform::getScale() const
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
		Vector3 qv = rotation.qv;
		float qw = rotation.qw;

		// Create quaternion matrix 
		Matrix4 rotation_matrix;

		rotation_matrix[0][0] = 1 - 2 * (qv.y * qv.y + qv.z * qv.z);
		rotation_matrix[0][1] = 2 * (qv.x * qv.y - qw * qv.z);
		rotation_matrix[0][2] = 2 * (qv.x * qv.z + qw * qv.y);
		rotation_matrix[0][3] = 0.f;

		rotation_matrix[1][0] = 2 * (qv.x * qv.y + qw * qv.z);
		rotation_matrix[1][1] = 1 - 2 * (qv.x * qv.x + qv.z * qv.z);
		rotation_matrix[1][2] = 2 * (qv.y * qv.z - qw * qv.x);
		rotation_matrix[1][3] = 0.f;

		rotation_matrix[2][0] = 2 * (qv.x * qv.z - qw * qv.y);
		rotation_matrix[2][1] = 2 * (qv.y * qv.z + qw * qv.x);
		rotation_matrix[2][2] = 1 - 2 * (qv.x * qv.x + qv.y * qv.y);
		rotation_matrix[2][3] = 0.f;

		rotation_matrix[3][0] = 0.f;
		rotation_matrix[3][1] = 0.f;
		rotation_matrix[3][2] = 0.f;
		rotation_matrix[3][3] = 1.f;

		return rotation_matrix;
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