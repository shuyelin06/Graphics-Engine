#pragma once

#include <map>
#include <vector>
#include <typeindex>

#include "Transform.h"
#include "math/Matrix4.h"
#include "math/Vector3.h"

namespace Engine
{
namespace Datamodel
{
	// Forward Declaration of Component
	class Component;

	// Object Class
	// Stores data regarding a generic object in our engine.
	// Every object has a parent and children. Together, their parent / child
	// relationships form an entire scene.
	// Unique behaviors for objects are implemented using components, which can be registered.
	class Object
	{
	protected:
		// Parent & Children for the Object
		Object* parent;
		std::vector<Object*> children;

		// Transform of the object
		Transform transform;

		// (Cached) Local --> World Matrix
		Math::Matrix4 m_local;
		
		// Additional Components for the Object
		std::map<std::type_index, void *> components;

	public:
		// Constructor & Destructor
		Object();
		~Object();

		// Object Hierarchy Methods
		Object* getParent() const; // Can return nullptr if parent does not exist
		std::vector<Object*>& getChildren();
		
		Object& createChild();

		// Transform Methods
		Transform& getTransform();

		const Math::Matrix4& getLocalMatrix() const;
		const Math::Matrix4& updateLocalMatrix(const Math::Matrix4& m_parent);
		
		// Component Handling Methods
		// GetComponent: Return a pointer to the component (of some type) from the object.
		// If the object does not have such a component, returns nullptr.
		template <typename ComponentType>
		ComponentType* getComponent() const
		{
			// Find the type's unique index
			std::type_index index = std::type_index(typeid(ComponentType));

			// Use this unique index to search the object's component map.
			if (components.contains(index))
				return static_cast<ComponentType*>(components[index]);
			else
				return nullptr;
		}

		// RegisterComponent:
		// Registers a component under this object's component list. Whenever a component
		// is created, it should be registered under its respective object.
		// Returns true on success, false on failure. Failure can happen if such a component
		// already exists.
		template <typename ComponentType>
		bool registerComponent(ComponentType* component)
		{
			// Find the type's unique index
			std::type_index index = std::type_index(typeid(ComponentType));

			// Use this unique index to search the component map.
			// If such a component doesn't exist, insert it into the map
			// so it can be accessed by other components, and set the "object" pointer
			// in the component.
			// Otherwise, fail and do nothing.
			if (!components.contains(index))
			{
				components[index] = component;
				return true;
			}
			else
				return false;
		}
		
		// RemoveComponent:
		// Removes a component from the object's component list. While doing this, calls the
		// component's destructor, which should remove it from the subsystems it is
		// subscribed to.
		// Returns true on success, false on failure. Failure can happen if the component
		// does not exist.
		template <typename ComponentType>
		bool removeComponent()
		{
			// Find the type's unique index
			std::type_index index = std::type_index(typeid(ComponentType));

			// Use this unique index to search the component map.
			// If such a component exists, remove it from the map.
			if (components.contains(index))
			{
				// Call component destructor. This expects the component to clean up
				// any links it has to other components (or systems).
				delete components[index];
				// Remove component from map
				components.erase(index);
				return true;
			}
			else
				return false;
		}

	};
}
}