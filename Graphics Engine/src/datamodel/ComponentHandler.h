#pragma once

#include <vector>

namespace Engine
{
namespace Datamodel
{
	// ComponentHandler Class:
	// Contains functionality for automatic management of components.
	// To implement a component, subsystems should inherit this class
	// for their particular component type.
	// We call methods of this class to create components. Components will automatically
	// call methods of this class to remove themselves.
	template<typename ComponentType>
	class ComponentHandler
	{
	protected:
		std::vector<ComponentType*> components;

	public:
		// Constructor
		// Initializes the components vector.
		ComponentHandler()
		{
			components = std::vector<ComponentType*>();
		}

		// CreateComponent:
		// Creates a component of a specified type.
		ComponentType* createComponent()
		{
			ComponentType* component = new ComponentType(this);
			components.push_back(component);
			return component;
		}

		// RemoveComponent:
		// Removes a component from the handler.
		bool removeComponent(ComponentType* component)
		{
			auto index = std::find(components.begin(), components.end(), component);

			if (index != components.end())
			{
				components.erase(index);
				return true;
			}
			else
				return false;
		}
	};

}
}