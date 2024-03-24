#pragma once

#include "math/Vector3.h"

namespace Engine
{
using namespace Math;
namespace Datamodel
{
	// Light Class:
	// Contains data and methods regarding a light in the scene,
	// which can illuminate the scene
	class Light
	{
	private:
		Vector3 position;

	public:
		Light();
		Light(float x, float y, float z);
	};
}
}