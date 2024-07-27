#include "Component.h"

namespace Engine
{
namespace Datamodel
{
	// Constructor:
	// Simply saves the pointer to the parent object
	Component::Component(Object* parent_object)
	{
		object = parent_object;
	}

	Object* Component::getObject()
	{
		return object;
	}

}
}