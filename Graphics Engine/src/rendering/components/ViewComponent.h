#pragma once

#include "datamodel/ComponentHandler.h"
#include "datamodel/Component.h"

namespace Engine
{
namespace Graphics
{
	// Forward Declare of VisualSystem
	class VisualSystem;

	// CameraComponent Class:
	// Allows a scene to be rendered from the POV of the object.
	// The POV of any object is given as its transform.
	class ViewComponent : 
		public Datamodel::Component<ViewComponent>
	{
	protected:
		// Cached matrix to avoid recomputation every frame
		Matrix4 projection_matrix;

		// Camera attributes
		float fov;

		float z_near;
		float z_far;

	public:
		ViewComponent(Datamodel::ComponentHandler<ViewComponent>* handler);

		// Get the local -> camera space matrix
		const Matrix4& getProjectionMatrix(void) const;

		// Get the camera's attributes
		float getFOV() const;
		float getZNear() const;
		float getZFar() const;

		// Set the camera's attributes
		void setFOV(float new_fov);
		void setZNear(float new_znear);
		void setZFar(float new_zfar);

	private:
		// Generate the local -> camera space matrix
		void generateProjectionMatrix(void);
	};
}
}