#pragma once

#include "Object.h"

namespace Engine
{
namespace Datamodel
{
	// Component Class:
	// Represents a component which can be assigned to an object
	// to define what that object is. 
	// - Component creation is expected to be done by the system
	//	 they belong to, which also registers the component under its object.
	// - Component deletion is expected to be done by the object
	//   they're assigned to, which also removes the component
	//   from its system. 
	// System deletion is expected to be manually done by the associated 
	// component destructor. 
	// The systems are responsible for creating components, and registering them / binding
	// them to their object. 
	// The objects are responsible for removing components, where the component should remove itself
	// from its parent system.
	// It's possible to use templates to do this, but I'm opting not to as using templates just made everything
	// a lot more complicated.
	class Component
	{
	protected:
		Object* object;

	public:
		// Constructor
		// Assigns a reference to the handler that created this component
		Component(Object* parent_object);

		Object* getObject();
	};
}
}