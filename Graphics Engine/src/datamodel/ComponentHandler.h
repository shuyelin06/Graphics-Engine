#pragma once

#include "Object.h"

namespace Engine {
namespace Datamodel {
// Class ComponentHandler:
// Represents a handler for components that
// a system can use for automatic component management.
// These handlers will automatically remove invalid
// components.
template <typename T> class ComponentHandler {
  private:
    std::vector<T*> components;

  public:
    ComponentHandler() : components() {}

    // Return the component vector
    const std::vector<T*>& getComponents() { return components; }

    // Add a new component to the handler
    void newComponent(Object* object, T* component) {
        // Bind my component to the object
        Component* comp = static_cast<Component*>(component);
        int index = object->bindComponent(comp);

        // Add to our components vector for tracking
        if (index != -1)
            components.push_back(component);
    }

    // Remove a component from the handler
    void removeComponent(int index) {
        delete components[index];
        components.erase(components.begin() + index);
    }

    // This will clear all components marked as invalid, and call
    // update for the remaining valid components.
    // Call in the beginning of a system's "prepare" method.
    void cleanAndUpdate() {
        int head = 0;

        for (int i = 0; i < components.size(); i++) {
            Component* component = static_cast<Component*>(components[i]);

            if (component->isValid()) {
                components[head++] = static_cast<T*>(component);
                component->update();
            } else {
                delete component;
            }
        }

        components.resize(head);
    }
};

} // namespace Datamodel
} // namespace Engine