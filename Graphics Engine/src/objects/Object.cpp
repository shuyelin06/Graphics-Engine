#include "Object.h"

#include <math.h>

namespace Engine
{
using namespace Math;

namespace Datamodel
{
	/* --- Constructors --- */
	// Constructor:
	// Creates an object with no parent and a
	// local position of (0,0,0)
	Object::Object()
	{
		parent = nullptr;
		rotation = Vector3();
		position_local = Vector3();
	}

	/* --- Operations --- */
	// SetPosition:
	// Sets the Object's position
	void Object::setPosition(float x, float y, float z)
	{
		position_local = Vector3(x, y, z);
	}
	
	// Offsets the local X position
	void Object::offsetX(float offset)
	{
		position_local.x += offset;
	}

	// Offsets the local Y position
	void Object::offsetY(float offset)
	{
		position_local.y += offset;
	}

	// Offsets the local Z position
	void Object::offsetZ(float offset)
	{
		position_local.z += offset;
	}

	// SetRotation:
	// Sets the Object's rotation
	void Object::setRotation(float roll, float yaw, float pitch)
	{
		rotation = Vector3(roll, yaw, pitch);
	}

	// Offsets the roll rotation of the object
	void Object::offsetRoll(float offset)
	{
		rotation.x += offset;
	}

	// Offsets the yaw rotation of the object
	void Object::offsetYaw(float offset)
	{
		rotation.y += offset;
	}

	// Offsets the pitch rotation of the object
	void Object::offsetPitch(float offset)
	{
		rotation.z += offset;
	}

	// LocalToWorldMatrix:
	// Returns the 4x4 matrix that, given a local point (like in a mesh), will
	// rotate and translate it to the world space as defined by 
	// the object's position and rotation.
	Matrix4 Object::localToWorldMatrix(void) // TODO: Actually Local to World
	{
		// Generate the Rotation Matrices
		// Rotation about the x-axis (roll)
		Matrix4 roll = Matrix4(
			1, 0, 0, 0,
			0, cosf(rotation.x), sinf(rotation.x), 0,
			0, -sinf(rotation.x), cosf(rotation.x), 0,
			0, 0, 0, 1
		);
		// Rotation about the y-axis (pitch)
		Matrix4 pitch = Matrix4(
			cosf(rotation.y), 0, -sinf(rotation.y), 0,
			0, 1, 0, 0,
			sinf(rotation.y), 0, cosf(rotation.y), 0,
			0, 0, 0, 1
		);
		// Rotation about the z-axis (yaw)
		Matrix4 yaw = Matrix4(
			cosf(rotation.z), sinf(rotation.z), 0, 0,
			-sinf(rotation.z), cosf(rotation.z), 0, 0,
			0, 0, 1, 0,
			0, 0, 0, 1
		);
		
		Matrix4 rotation_matrix = roll * pitch * yaw;

		// Generate Translation Matrix
		Matrix4 translation_matrix = Matrix4();
		Vector3 world_position = worldPosition();

		translation_matrix[0][0] = 1;
		translation_matrix[1][1] = 1;
		translation_matrix[2][2] = 1;
		translation_matrix[3][3] = 1;

		translation_matrix[3][0] = world_position.x; // Apply x translation
		translation_matrix[3][1] = world_position.y; // Apply y translation
		translation_matrix[3][2] = world_position.z; // Apply z translation

		// Build Transformation Matrix
		Matrix4 local_to_world = rotation_matrix * translation_matrix; // Apply 

		return local_to_world;
	}
	
	// WorldPosition:
	// Return the object's world position. This is found by 
	// recursively adding the position of the object's parents together.
	Vector3 Object::worldPosition()
	{
		if (parent == nullptr)
		{
			return position_local;
		}
		else
		{
			return parent->worldPosition() + position_local;
		}
	}


}
}