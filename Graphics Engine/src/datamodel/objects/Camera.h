#pragma once

#include "../geometry/Vector3.h"

namespace Engine 
{
	class Camera
	{
		Vector3 position; // x, y, z
		Vector3 direction; // roll, yaw, pitch

		float field_of_view;

		Vector3 toCameraSpace(const Vector3&);
	};
}