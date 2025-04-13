#pragma once

#include <string>
#include <unordered_map>

#define COMPONENT_TAG_NONE 0

namespace Engine {
namespace Datamodel {
class Object;

// Component Class:
// Represents a basic component of an object that
// can be used to give it additional behaviors.
// Components provide a unified interface to work
// with objects.
class Component {
  private:
    static std::unordered_map<std::string, unsigned int> tag_map;
    static unsigned int new_tag;

  protected:
    // Once set, this pointer cannot change.
    Object* object;

    // Unique tag for a component.
    unsigned int tag;

    // A flag indicating if the component is valid or not.
    // If the component is invalid, the system handling it
    // should ignore / destroy the component.
    bool valid;

  public:
    Component(Object* object);
    ~Component();

    bool isValid() const;
    unsigned int getTag() const;

    const Object* getObject() const;
    Object* getObject();

    void markInvalid();

    // Generate a new tag. Systems that want to create components
    // should register them here, so that each component has a unique tag.
    static unsigned int registerNewTag(const std::string& component_name);
    static unsigned int getTag(const std::string& name);
};

} // namespace Datamodel
} // namespace Engine