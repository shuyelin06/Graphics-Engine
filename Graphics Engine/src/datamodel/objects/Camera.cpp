#include "Camera.h"

#include "../geometry/Matrix3.h"
#include <math.h>

namespace Engine
{
	Vector3 Camera::toCameraSpace(const Vector3& coords)
	{
		// create rotation matrix for camera direction
		Matrix3 roll = Matrix3(
			cos(direction.x), sin(direction.x), 0,
			-sin(direction.x), cos(direction.x), 0,
			0, 0, 1
		);
		Matrix3 yaw = Matrix3(
			1, 0, 0,
			0, cos(direction.y), sin(direction.y),
			0, -sin(direction.y), cos(direction.y)
		);
		Matrix3 pitch = Matrix3(
			cos(direction.z), sin(direction.z), 0,
			-sin(direction.z), cos(direction.z), 0,
			0, 0, 1
		);

		Matrix3 rotation_matrix = roll * yaw * pitch;
		
		// Invert to transform world space to camera space
		Matrix3 inverse = rotation_matrix.inverse();

		// Transform
		return inverse * (coords - position);
	}
}