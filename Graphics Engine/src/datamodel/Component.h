#pragma once

#include "ComponentHandler.h"
#include "Object.h"

namespace Engine
{
namespace Datamodel
{
	// Component Class:
	// Contains generic methods and fields that a component will
	// always have access to.
	// Components are managed by the object they are registered under. 
	// Upon destruction, the component will automatically remove itself
	// from the system that created it.
	// When creating a component, it should inherit from Component<Type>,
	// where "Type" is the subclass name itself.
	template <typename Type>
	class Component
	{
	protected:
		ComponentHandler<Type>* handler;

		// Allow direct object access, to update the pointer
		// accordingly. 
		friend class Object;
		Object* object;

	public:
		// Constructor
		// Assigns a reference to the handler that created this component
		Component(ComponentHandler<Type>* _handler)
		{
			handler = _handler;
		}
		
		// Destructor
		// Automatically removes this component from the handler
		~Component()
		{
			handler->removeComponent(static_cast<Type*>(this));
		}

		// Gets the object associated with this component.
		const Object* getObject()
		{
			return object;
		}
	};
}
}