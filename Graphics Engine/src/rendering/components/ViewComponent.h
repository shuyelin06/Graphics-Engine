#pragma once

#include "VisualComponent.h"

#include "rendering/Shader.h"

namespace Engine
{
namespace Graphics
{
	// Forward Declare of VisualSystem
	class VisualSystem;

	// ViewData:
	// Data to be loaded into the per-view constant buffer
	struct ViewData
	{
		Matrix4 view_matrix;
		Matrix4 projection_matrix;
	};

	// CameraComponent Class:
	// Allows a scene to be rendered from the POV of the object.
	// The POV of any object is given as its transform.
	class ViewComponent : public VisualComponent
	{
	protected:
		VisualSystem* system;

		// Camera attributes
		float fov;

		float z_near;
		float z_far;

	public:
		ViewComponent(Datamodel::Object* object, VisualSystem* system);
		~ViewComponent();

		// Given a visual system, loads per-view data
		// into constant buffer 1.
		void loadViewData(VisualSystem* system, CBHandle* handle) const;

		// Get the local -> camera space matrix
		const Matrix4 getProjectionMatrix(void) const;

		// Get the camera's attributes
		float getFOV() const;
		float getZNear() const;
		float getZFar() const;

		// Set the camera's attributes
		void setFOV(float new_fov);
		void setZNear(float new_znear);
		void setZFar(float new_zfar);

	protected:
		// Generate the local -> camera space matrix
		Matrix4 generateProjectionMatrix(void) const;
	};
}
}