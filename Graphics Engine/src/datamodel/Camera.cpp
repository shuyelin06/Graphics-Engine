#include "Camera.h"

#include "math/Compute.h"

#include <math.h>

#define ASPECT_RATIO (1920.f / 1080.f)

namespace Engine
{
using namespace Math;

namespace Datamodel
{
	/* --- Constructors --- */
	Camera::Camera() : Object()
	{
		setFOV(1.2f);

		x_theta = y_theta = 0.f;

		z_near = 1.f;
		z_far = 200.f;
	}
	
	Camera::Camera(float fov) : Object() 
	{
		setFOV(fov);

		z_near = 1.f;
		z_far = 200.f;
	}

	float Camera::getFOV()
	{
		return fov;
	}

	void Camera::setFOV(float new_fov)
	{
		fov = Compute::clamp(new_fov, 0.5f, PI - 0.5f);
	}

	// OffsetView:
	// Offsets the camera's viewing angle by theta radians x, 
	// theta radians y (from the camera's perspective)
	void Camera::offsetViewingAngle(float _x_delta, float _y_delta)
	{
		// Update x theta
		parent->getTransform()->offsetRotation(Vector3::PositiveY(), _x_delta);
		x_theta = x_theta + _x_delta;

		// Update y theta
		float new_y_theta = Compute::clamp(y_theta + _y_delta, -PI / 4, PI / 4);
		_y_delta = new_y_theta - y_theta;
		y_theta = new_y_theta;
		transform.offsetRotation(Vector3::PositiveX(), _y_delta);
	}

	// Calculate camera's forward viewing vector
	Vector3 Camera::forward()
	{
		// TODO: Movement is off
		// Get rotation matrix
		Matrix4 m1 = transform.rotationMatrix();
		Matrix4 m2 = parent->getTransform()->rotationMatrix();
		Matrix4 rotation_matrix = m2 * m1;

		// Camera is by default looking in the +Z axis
		Vector4 view = rotation_matrix * Vector4::PositiveZW();
		return view.toVector3();
	}

	// Calculate camera's right viewing vector
	Vector3 Camera::right()
	{
		// Get rotation matrix
		Matrix4 m1 = transform.rotationMatrix();
		Matrix4 m2 = parent->getTransform()->rotationMatrix();
		Matrix4 rotation_matrix = m2 * m1;

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