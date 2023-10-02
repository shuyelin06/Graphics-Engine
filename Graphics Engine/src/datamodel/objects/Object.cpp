#include "Object.h"

namespace Engine
{
using namespace Math;

namespace Datamodel
{
	/* --- Constructors --- */
	// Constructor:
	// Creates an object with no parent and a
	// local position of (0,0,0)
	Object::Object()
	{
		parent = nullptr;
		position_local = Vector3();
	}

	/* --- Operations --- */
	// SetPosition:
	// Sets the Object's position
	void Object::setPosition(float x, float y, float z)
	{
		position_local = Vector3(x, y, z);
	}

	// WorldPosition:
	// Return the object's world position. This is found by 
	// recursively adding the position of the object's parents together.
	Vector3 Object::worldPosition()
	{
		if (parent == nullptr)
		{
			return position_local;
		}
		else
		{
			return parent->worldPosition() + position_local;
		}
	}


}
}