#pragma once

#include "Object.h"

namespace Engine
{
namespace Datamodel
{
	// Light Class:
	// Contains data and methods regarding a light in the scene,
	// which can illuminate the scene
	// All lights are objects, but with additional properties
	class Light : public Object
	{
	public:
		Light();
	};
}
}