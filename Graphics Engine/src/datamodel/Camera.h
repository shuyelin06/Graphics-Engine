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
		// Camera viewing direction in radians, relative
		// to how the camera sees the scene
		float x_theta, y_theta;

		float fov;

		float z_near;
		float z_far;

	public:
		Camera();
		Camera(float fov);

		float getFOV();
		void setFOV(float new_fov);

		// Change viewing angle
		void offsetViewingAngle(float x_delta, float y_delta);

		// Overridden rotation method that will clamp the angle
		void offsetXRotate(float theta);
		void offsetYRotate(float theta);

		// Directional viewing vectors
		Vector3 forward();	// Camera forward vector
		Vector3 right();	// Camera right vector

		Matrix4 localToProjectionMatrix(void);
	};
}
}