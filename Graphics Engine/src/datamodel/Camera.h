#pragma once

#include "datamodel/Object.h"

#include "math/Vector3.h"
#include "math/Vector4.h"
#include "math/Matrix4.h"

using namespace Engine::Math;

namespace Engine 
{
namespace Datamodel
{

	class Camera: public Object
	{
	private:
		// Local cache of the world -> camera matrix to avoid 
		// extra unnecessary computations
		Matrix4 cameraMatrix;

		float fov;

		float z_near;
		float z_far;

	public:
		Camera();
		Camera(float fov);

		// @Overriden
		// Handle Camera Movement
		void update();

		// Get the local -> camera space matrix
		const Matrix4& getCameraMatrix(void) const;

		// Get the camera's attributes
		float getFOV() const;
		float getZNear() const;
		float getZFar() const;

		// Set the camera's attributes
		void setFOV(float new_fov);
		void setZNear(float new_znear);
		void setZFar(float new_zfar);

		// Overridden rotation method that will clamp the angle
		void offsetRotation(float x, float y, float z);

		// Directional viewing vectors
		Vector3 forward();	// Camera forward vector
		Vector3 right();	// Camera right vector

		
	private:
		// Generate the local -> camera space matrix
		void generateCameraMatrix(void);
	};
}
}