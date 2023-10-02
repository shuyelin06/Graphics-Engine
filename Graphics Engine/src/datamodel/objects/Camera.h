#pragma once

#include "datamodel/objects/Object.h"
#include "math/Vector3.h"
#include "math/Matrix4.h"

using namespace Engine::Math;

namespace Engine 
{
namespace Datamodel
{

	class Camera: Object
	{
	public:
		Vector3 view_direction; // roll, yaw, pitch

		Camera();

		Matrix4 worldToCameraMatrix();
	};
}
}