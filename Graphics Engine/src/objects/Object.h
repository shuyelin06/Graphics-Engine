#pragma once

#include "math/Vector3.h"
#include "math/Matrix4.h"

namespace Engine
{
using namespace Math;

namespace Datamodel
{

	// Object
	// Stores data regarding a generic
	// object in our engine. 
	class Object
	{
	public:
		Object* parent;
		Vector3 rotation;		// roll, yaw, pitch
		Vector3 position_local; // x, y, z

		Object();

		void setPosition(float x, float y, float z);
		void offsetX(float offset);
		void offsetY(float offset);
		void offsetZ(float offset);

		void setRotation(float roll, float yaw, float pitch);
		void offsetRoll(float offset);
		void offsetYaw(float offset);
		void offsetPitch(float offset);

		Matrix4 localToWorldMatrix(void);

	private:
		Vector3 worldPosition(); // Helper
	};
}
}