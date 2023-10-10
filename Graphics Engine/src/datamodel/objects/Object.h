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
	// object in the graphics engine. 
	class Object
	{
	public:
		Object* parent;
		Vector3 position_local; // Local position (relative to parent)
		Vector3 rotation; // Rotation (euler angles)
		
		Object();

		void setPosition(float x, float y, float z);

		Vector3 worldPosition();

	protected:
		Matrix4 transformMatrix();
	};
}
}