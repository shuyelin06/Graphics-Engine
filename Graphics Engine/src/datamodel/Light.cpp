#include "Light.h"

namespace Engine
{
namespace Datamodel
{
	// Default Constructor:
	// Creates a light with position at the origin
	// (0,0,0)
	Light::Light()
	{
		position = Vector3(0,0,0);
	}

	// Constructor:
	// Creates a light with position at specified coordinates
	Light::Light(float x, float y, float z)
	{
		position = Vector3(x, y, z);
	}
}
}