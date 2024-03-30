#include "Object.h"

#include <math.h>

namespace Engine
{
using namespace Math;

namespace Datamodel
{
	ObjectAccessor::ObjectAccessor() {}

	/* --- Constructors --- */
	// Constructor:
	// Creates an object with no parent and a
	// local position of (0,0,0)
	Object::Object()
	{
		parent = nullptr;
		transform = Transform();
		visual_attr = nullptr;
	}

	/* --- Operations --- */
	// GetTransform:
	// Returns the object's transform property
	Transform* Object::getTransform(ObjectAccessor)
	{
		return &transform;
	}

	Transform* Object::getTransform()
	{
		return &transform;
	}

	// GetParent:
	Object* Object::getParent(ObjectAccessor)
	{
		return parent;
	}

	// SetParent:
	// Sets the Object's parent
	void Object::setParent(Object* _parent)
	{
		parent = _parent;
	}

	// SetVisualAttribute
	// Sets the object's visual attribute
	void Object::setVisualAttribute(Graphics::VisualAttribute* _attr)
	{
		visual_attr = _attr;
	}

}
}