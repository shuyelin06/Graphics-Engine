#include "Camera.h"

#include <math.h>

namespace Engine
{
using namespace Math;

namespace Datamodel
{
	/* --- Constructors --- */
	Camera::Camera()
	{
		view_direction = Vector3();
	}

	/* --- Operators --- */
	// WorldToCameraMatrix:
	// Returns the Matrix4 matrix which, given a world
	// coordinate, will transform it into the camera space.
	Matrix4 Camera::worldToCameraMatrix()
	{
		// Find camera world position
		Vector3 world_position = worldPosition();

		// Find the rotation matrices
		Matrix4 roll_matrix = Matrix4(
			cosf(view_direction.x), sinf(view_direction.x), 0, 0,
			-sinf(view_direction.x), cosf(view_direction.x), 0, 0,
			0, 0, 1, 0,
			0, 0, 0, 0
		);
		Matrix4 yaw_matrix = Matrix4(
			1, 0, 0, 0,
			0, cosf(view_direction.y), sinf(view_direction.y), 0,
			0, -sinf(view_direction.y), cosf(view_direction.y), 0,
			0, 0, 0, 0
		);
		Matrix4 pitch_matrix = Matrix4(
			cosf(view_direction.z), sinf(view_direction.z), 0, 0,
			-sinf(view_direction.z), cosf(view_direction.z), 0, 0,
			0, 0, 1, 0,
			0, 0, 0, 0
		);

		// Build the camera matrix. This is the camera to world space matrix.
		Matrix4 camera_matrix = roll_matrix * yaw_matrix * pitch_matrix;
		
		camera_matrix[0][3] = world_position.x;
		camera_matrix[1][3] = world_position.y;
		camera_matrix[2][3] = world_position.z;
		camera_matrix[3][3] = 1;

		// Take the inverse to find the world to camera space matrix.
		return camera_matrix.inverse();
	}
}
}