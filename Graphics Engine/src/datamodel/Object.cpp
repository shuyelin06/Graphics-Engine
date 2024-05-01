#include "Object.h"

#include <math.h>

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
		mesh = nullptr;
	}

	/* --- Operations --- */
	// GetTransform:
	// Returns the object's transform property
	Transform* Object::getTransform()
	{
		return &transform;
	}

	// SetParent:
	// Sets the Object's parent
	void Object::setParent(Object* _parent)
	{
		parent = _parent;
	}

	// SetMesh
	// Sets the object's mesh
	void Object::setMesh(Mesh* _mesh)
	{
		mesh = _mesh;
	}

}
}