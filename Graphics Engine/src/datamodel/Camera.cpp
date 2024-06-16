#include "Camera.h"

#include "input/callbacks/InputPoller.h"
#include "math/Compute.h"

#include <math.h>

#define ASPECT_RATIO (1920.f / 1080.f)

namespace Engine
{
using namespace Input;
using namespace Math;

namespace Datamodel
{
	/* --- Constructors --- */
	Camera::Camera() : Object(), cameraMatrix()
	{
		setFOV(1.2f);

		z_near = 1.f;
		z_far = 200.f;

		generateCameraMatrix();
	}
	
	Camera::Camera(float fov) : Object() 
	{
		setFOV(fov);

		z_near = 1.f;
		z_far = 200.f;

		generateCameraMatrix();
	}

	// --- Accessors ---
	// GetCameraMatrix:
	// Returns a reference to the camera's precomputed world -> camera
	// space matrix.
	const Matrix4& Camera::getCameraMatrix(void) const
	{
		return cameraMatrix;
	}

	// GetFov: 
	// Returns the camera's FOV
	float Camera::getFOV() const
	{
		return fov;
	}
	
	// GetZNear:
	// Returns the distance the Z-Near plane is from the camera. 
	// Anything closer to the camera than this is clipped.
	float Camera::getZNear() const
	{
		return z_near;
	}

	// GetZFar:
	// Returns the distance the Z-Far plane is from the camera. 
	// Anything further from the camera than this is clipped.
	float Camera::getZFar() const
	{
		return z_far;
	}

	// --- Setters ---
	// Setters will automatically regenerate the camera's matrix to
	// avoid recomputation
	
	// SetFOV:
	// Set's the camera's FOV. Clamped to prevent excessively wide
	// FOVs. 
	void Camera::setFOV(float new_fov)
	{
		fov = Compute::clamp(new_fov, 0.5f, PI - 0.5f);
		generateCameraMatrix();
	}

	// SetZNear:
	// Set the distance of the Z-Near plane.
	void Camera::setZNear(float new_znear)
	{
		z_near = new_znear;
		generateCameraMatrix();
	}

	// SetZFar:
	// Set the distance of the Z-Far plane 
	void Camera::setZFar(float new_zfar)
	{
		z_far = new_zfar;
		generateCameraMatrix();
	}

	// OffsetRotation:
	// Offsets the camera's rotation, while clamping it
	void Camera::offsetRotation(float x, float y, float z)
	{
		const Vector3 rotation = transform.getRotation();
		transform.setRotation(Compute::clamp(rotation.x + x, -PI / 2, PI / 2),
							  rotation.y + y,
							  0);
	}

	// Update:
	// Polls the input system to update the camera position
	void Camera::update()
	{
		// Movement direction for the camera
		Vector3 movement_direction = Vector3();

		// Poll the input system to determine the movement direction for 
		// the camera
		if (Input::InputPoller::IsSymbolActive('w'))
			movement_direction += forward();
		if (Input::InputPoller::IsSymbolActive('a'))
			movement_direction -= right();
		if (Input::InputPoller::IsSymbolActive('s'))
			movement_direction -= forward();
		if (Input::InputPoller::IsSymbolActive('d'))
			movement_direction += right();

		// If not 0, normalize and update the position
		if (movement_direction.magnitude() != 0)
		{
			movement_direction.inplaceNormalize();
			velocity += movement_direction * 10.f;
		}
	}

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



	

	// GenerateCameraMatrix:
	// Generates the local -> camera space matrix. Separating this
	// from the matrix retrieval lets us avoid recomputing this matrix
	// more than necessary. This is called internally when the camera is updated.
	void Camera::generateCameraMatrix(void)
	{
		float fov_factor = cosf(fov / 2.f) / sinf(fov / 2.f);

		cameraMatrix[0][0] = fov_factor / ASPECT_RATIO;
		cameraMatrix[1][1] = fov_factor;
		cameraMatrix[2][2] = z_far / (z_far - z_near);
		cameraMatrix[2][3] = 1;
		cameraMatrix[3][2] = (z_near * z_far) / (z_near - z_far);
	}

}
}