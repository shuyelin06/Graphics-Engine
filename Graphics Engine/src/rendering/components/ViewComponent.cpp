#include "ViewComponent.h"

#include <math.h>

#include "math/Compute.h"
#include "rendering/VisualSystem.h"

#define ASPECT_RATIO (1920.f / 1080.f)

namespace Engine
{
namespace Graphics
{
	// Constructor
	// Initializes the camera component with some default values.
	ViewComponent::ViewComponent(Datamodel::ComponentHandler<ViewComponent>* _handler)
		: Datamodel::Component<ViewComponent>(handler)
	{
		fov = 1.2f;
		z_near = 1.f;
		z_far = 200.f;

		generateProjectionMatrix();
	}

	// --- Accessors ---
	// GetCameraMatrix:
	// Returns a reference to the camera's precomputed world -> camera
	// space matrix.
	const Matrix4& ViewComponent::getProjectionMatrix(void) const
	{
		return projection_matrix;
	}

	// GetFov: 
	// Returns the camera's FOV
	float ViewComponent::getFOV() const
	{
		return fov;
	}

	// GetZNear:
	// Returns the distance the Z-Near plane is from the camera. 
	// Anything closer to the camera than this is clipped.
	float ViewComponent::getZNear() const
	{
		return z_near;
	}

	// GetZFar:
	// Returns the distance the Z-Far plane is from the camera. 
	// Anything further from the camera than this is clipped.
	float ViewComponent::getZFar() const
	{
		return z_far;
	}

	// --- Setters ---
	// Setters will automatically regenerate the camera's matrix to
	// avoid recomputation

	// SetFOV:
	// Set's the camera's FOV. Clamped to prevent excessively wide
	// FOVs. 
	void ViewComponent::setFOV(float new_fov)
	{
		fov = Compute::clamp(new_fov, 0.5f, PI - 0.5f);
		generateProjectionMatrix();
	}

	// SetZNear:
	// Set the distance of the Z-Near plane.
	void ViewComponent::setZNear(float new_znear)
	{
		z_near = new_znear;
		generateProjectionMatrix();
	}

	// SetZFar:
	// Set the distance of the Z-Far plane 
	void ViewComponent::setZFar(float new_zfar)
	{
		z_far = new_zfar;
		generateProjectionMatrix();
	}

	// --- Private Helpers --- 
	// GenerateCameraMatrix:
	// Generates the local -> camera space matrix. Separating this
	// from the matrix retrieval lets us avoid recomputing this matrix
	// more than necessary. This is called internally when the camera is updated.
	void ViewComponent::generateProjectionMatrix(void)
	{
		float fov_factor = cosf(fov / 2.f) / sinf(fov / 2.f);

		projection_matrix[0][0] = fov_factor / ASPECT_RATIO;
		projection_matrix[1][1] = fov_factor;
		projection_matrix[2][2] = z_far / (z_far - z_near);
		projection_matrix[2][3] = 1;
		projection_matrix[3][2] = (z_near * z_far) / (z_near - z_far);
	}
}
}