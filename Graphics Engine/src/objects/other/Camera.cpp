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
	Camera::Camera(float fov) {
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