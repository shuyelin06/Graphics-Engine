#include "Camera.h"

#include "math/Utility.h"

#include <math.h>

#define ASPECT_RATIO (1920.f / 1080.f)

namespace Engine
{
using namespace Math;

namespace Datamodel
{
	/* --- Constructors --- */
	Camera::Camera(float fov) : Object() {
		setFOV(fov);

		z_near = 1.f;
		z_far = 50.f;
	}

	float Camera::getFOV()
	{
		return fov;
	}

	void Camera::setFOV(float new_fov)
	{
		fov = Utility::clamp(new_fov, 0.5f, PI - 0.5f);
	}

	void Camera::offsetRotation(float x, float y, float z)
	{
		rotation.x = Utility::clamp(rotation.x + x, -PI / 2, PI / 2);
		rotation.y += y;
		rotation.z = 0; // Does nothing
	}

	// Calculate camera's forward viewing vector
	Vector3 Camera::forward()
	{
		// Get rotation matrix
		Matrix4 rotation_matrix = rotationMatrix().tranpose();

		// Camera is by default looking in the +Z axis
		Vector4 view = rotation_matrix * Vector4::PositiveZW();
		return view.toVector3();
	}

	// Calculate camera's right viewing vector
	Vector3 Camera::right()
	{
		// Get rotation matrix
		Matrix4 rotation_matrix = rotationMatrix().tranpose();

		// Camera's left is by default looking in the +X axis
		Vector4 view = rotation_matrix * Vector4::PositiveXW();
		return view.toVector3();
	}

	// Returns the matrix converting local coordinates (in camera space)
	// to the projection space, so they can be rendered.
	Matrix4 Camera::localToProjectionMatrix(void)
	{
		Matrix4 local_to_project = Matrix4();
		
		float fov_factor = cosf(fov / 2.f) / sinf(fov / 2.f);

		local_to_project[0][0] = fov_factor / ASPECT_RATIO;
		local_to_project[1][1] = fov_factor;
		local_to_project[2][2] = z_far / (z_far - z_near);
		local_to_project[2][3] = 1;
		local_to_project[3][2] = (z_near * z_far) / (z_near - z_far);

		return local_to_project;
	}

}
}