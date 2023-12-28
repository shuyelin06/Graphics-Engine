#pragma once

#include "objects/Object.h"

#include "math/Vector3.h"
#include "math/Matrix4.h"

using namespace Engine::Math;

namespace Engine 
{
namespace Datamodel
{

	class Camera: public Object
	{
	private:
		float fov;

		float z_near;
		float z_far;

	public:
		Camera(float fov);

		float getFOV();
		void setFOV(float new_fov);

		Matrix4 localToProjectionMatrix(void);
	};
}
}